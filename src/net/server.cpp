#include "net/server.hpp"
#include "net/socket.hpp"
#include "protocol/message.hpp"
#include <cerrno>
#include <iostream>

/* library for using epoll */
#include <sys/epoll.h>
#define NUM_EVENTS 64
#define BUFF_SIZE 1024


namespace net {

    Server::Server(uint16_t port_number) {
        init_sockets();

        // set port number
        port = port_number;

        last_uid = 1;

        // initialize the server socket
        server_socket = net::create_tcp_socket();

        // bind the server socket to the port
        bind_socket(server_socket, port);

        // I think for now, the unordered_sets are auto initialized as empty

    }

    Server::~Server(){
        // TODO: probably you should gracefully close all the connections
        // TODO: probably clean up any new memory
        cleanup_sockets();
    }

    void Server::handle_message(socket_t conn, protocol::Message msg) {
        // if it's an authentication, add it to the username mapping
        if (msg.type == protocol::MessageType::Auth) {

            // get the username
			std::string username = protocol::deserialize_auth(msg);
            
            // generate a new uid
            uid_t uid = gen_uid();

            // update the mappings accordingly
            client_uids[conn] = uid;
            uid_to_socket[uid] = conn;
            client_usernames[uid] = username;

            // if a user has authorized, send the UID to the user
            std::vector<uint8_t> user_info = protocol::serialize_message(
                protocol::serialize_user_info(uid, username)
            );
            net::send_all(conn, user_info.data(), user_info.size());


        } else if (msg.type == protocol::MessageType::Chat) {
            auto [sender, receiver, message] = protocol::deserialize_chat(msg);
            
            // verify input and output uids
            // TODO: actually verify username info as well
            if (client_uids.contains(conn) && 
                uid_to_socket.contains(sender) &&
                client_uids[conn] == sender &&
                uid_to_socket.contains(receiver)                
            ) {
                
                // get the appropriate receiver socket
                socket_t receiver_socket = uid_to_socket[receiver];

                // reserialize message (IF ANY MODIFICATIONS NEED TO BE MADE, DO IT HERE)
                std::vector<uint8_t> output = protocol::serialize_message(msg);

                // pass along the message to the receiver socket
                net::send_all(receiver_socket, output.data(), output.size());
            }
        } else if (msg.type == protocol::MessageType::REQ_USER_ID) {
            // consider creating another map to get uids from username
            std::string request_username = protocol::deserialize_req_user_id(msg);
            for (const auto [uid, user] : client_usernames) {
                if (user == request_username) {
                    std::vector<uint8_t> output = protocol::serialize_message(
                        protocol::serialize_user_info(uid, user)
                    );
                    net::send_all(conn, output.data(), output.size());
                    break;
                }
            }
        } else if (msg.type == protocol::MessageType::REQ_USERNAME) {
            uid_t uid = protocol::deserialize_req_username(msg);
            if (client_usernames.contains(uid)) {
                std::vector<uint8_t> output = protocol::serialize_message(
                    protocol::serialize_user_info(uid, client_usernames[uid])
                );
                net::send_all(conn, output.data(), output.size());
            }
        }
    }


    // use the connection and buffer to correctly parse incoming bytes
    //
    // calls correct handler based on parsed message type
    // 
    // returns False if more bytes need to be read to be parsed, or if it can't be parsed.
    //
    // returns True if parsed correctly
    bool Server::buffered_read(socket_t conn, std::vector<uint8_t>& client_buffer) {
        bool flag = false;
        // while data is still readable from the socket...
        do {

            // read into the global buffer
            int bytes_read = receive(conn, global_buffer, SERVER_BUFFER_SIZE);

            // if there's an issue reading, do nothing
            // TODO: add appropriate handline if recv causes error, maybe break out of loop?
            if (bytes_read != -1) {
                // append global buffer data to the end of the client_buffer
                client_buffer.insert(client_buffer.end(), global_buffer, global_buffer + bytes_read);

                // if the size of the buffer is enough to read the length, read it and check if we can parse a message
                while (client_buffer.size() >= 4) {

                    // read the message length
                    uint32_t message_len {0};
                    message_len |= (client_buffer[0] << 24);
                    message_len |= (client_buffer[1] << 16);
                    message_len |= (client_buffer[2] << 8);
                    message_len |= (client_buffer[3]);

                    // ensure that length has proper endianness
                    message_len = ntohl(message_len);

                    // if we can read the entire message, parse and handle it
                    if (client_buffer.size() >= 4 + message_len) {
                        // pop the length from the buffer
                        client_buffer.erase(client_buffer.begin(), client_buffer.begin() + 4);

                        // parse the message into a message struct
                        protocol::Message msg = protocol::deserialize_message(message_len, client_buffer);

                        // clear the message from the buffer
                        client_buffer.erase(client_buffer.begin(), client_buffer.begin() + message_len);

                        // handle the parsed message TODO: consider using a thread to handle this?
                        handle_message(conn, msg);

                        // set the flag to true
                        flag = true;
                    }
                } 
            }
            

        } while (errno != EAGAIN && errno != EWOULDBLOCK);

        return flag;
    }

    // increments last_uid and returns the incremented value
    uid_t Server::gen_uid() {
        return ++last_uid;
    }

    void Server::start() {

        // create the epoll instance
        int epoll_fd = epoll_create1(0);

        // dedicated epoll event for registering new sockets
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = server_socket;

        // register the server socket onto the epoll instance
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ev);

        // vector of epoll events for new updates
        std::vector<epoll_event> events(NUM_EVENTS);

        // begin the listen loop
        net::listen_socket(server_socket);
        std::cout << "Listening on PORT: " << port << "\n";
        while(true) {
            int nfds;
            socket_t conn_sock;

            // wait for epoll events and get the number of returned fds in nfds
            nfds = epoll_wait(epoll_fd, events.data(), NUM_EVENTS, -1);

            // iterate through each event and handle appropriately
            for (int i = 0; i < nfds; i++) {

                // if returned fd is the server socket, this means there's a new connection
                if (events[i].data.fd == server_socket) {

                    // accept connection
                    conn_sock = accept_socket(server_socket);

                    // set the connection as nonblocking
                    set_non_blocking(conn_sock);

                    // register the connection socket to the epoll instance
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = conn_sock;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev);

                    // TODO: when registering new connection, you also need to generate a UID and send it
                    // register the new connection with the Server's maps, sets, etc
                    clients.insert(conn_sock);
                    client_buffers[conn_sock] = std::vector<uint8_t>();

                } else {

                    // TODO: for now, broadcast the message to all other 
                    socket_t sender_socket = events[i].data.fd;

                    // peek to see if connection has closed
                    uint8_t check_byte;
                    int check_conn = recv(sender_socket, &check_byte, 1, MSG_PEEK);
                    
                    // bytes read was 0, the connection closed gracefully (or -1 if error, for now handle the same way)
                    // TODO: add separate handling for a gracefully closed connection vs an error
                    if (check_conn <= 0) {
                        close_socket(sender_socket);
                        clients.erase(sender_socket);
                        client_buffers.erase(sender_socket);
                        client_usernames.erase(sender_socket);
                    } else {
                        buffered_read(sender_socket, client_buffers[sender_socket]);
                    }
                }
            }
        }
    }
}
