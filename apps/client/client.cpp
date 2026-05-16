#include "client/client.hpp"
#include "net/socket.hpp"
#include "protocol/message.hpp"
#include "client/appstate.hpp"

#include <iostream>
#include <string>
#include <string.h>
#include <sstream>

#define BUFF_SIZE 1024

namespace net {


    Client::Client(std::string host, std::string port, std::string user) {
        init_sockets();
        uid = INVALID_UID;

        username = user;

        // connect to the server
        server_conn = connect_to_host(host, port);
        if (server_conn == INVALID_SOCKET) {
            throw std::runtime_error("Connecting to host failed.\n");
        }

        // set the connection to be nonblocking
        bool non_block_success = set_non_blocking(server_conn);
        if (!non_block_success) {
            throw std::runtime_error("Could not set socket to non-blocking.\n");
        }

        // send an auth message
        // TODO: probably need some sort of confirmation/ UID generated
        std::vector<uint8_t> auth_message = serialize_message(
            serialize_auth(username)
        );
        send_all(server_conn, auth_message.data(), auth_message.size());
    }

    Client::~Client() {
        cleanup_sockets();
    }

    void Client::handle_message(Message msg, std::queue<Event> &e_queue, std::mutex &lock) {
        if (msg.type == MessageType::USER_INFO) {
            auto [recv_uid, recv_user] = deserialize_user_info(msg);
            std::cerr << "Attempting to map uid: " << recv_uid << " to: " << recv_user << "\n";
            // if uid is set to the `INVALID_UID` we assume that this is the first "request for user info"
            // which means that this is THIS client's UID
            // TODO: probably add better handling, or a separate handler to get the current client's uid
            if (username == recv_user) {
                std::cerr << "Setting own uid to: " << recv_uid << "\n";
                uid = recv_uid;
                std::cerr << "UID NOW: " << uid << "\n";
            
            // if uid is set, this is some other user, so we add it to our mapping
            } else {
                recv_user = std::to_string(recv_uid) + "_" + recv_user;
                push_event({EventType::NEW_CONTACT, recv_uid, recv_user}, e_queue, lock);
                username_map[recv_uid] = recv_user;
                std::cerr << "Added uid: " << std::to_string(recv_uid) << ", user: " << recv_user << "\n";
            }
        } else if (msg.type == MessageType::Chat) {
            auto [sender, receiver, output] = deserialize_chat(msg);
            std::cerr << "Received message from uid: " << sender << "\n";
            // first check if the sender has a known mapped username
            if (username_map.contains(sender)) {
                push_event({EventType::INC_MSG, sender, output}, e_queue, lock);
            }
        }
    }

    void Client::listen(bool *should_continue, std::queue<Event> &e_queue, std::mutex &lock) {
        std::vector<uint8_t> client_buffer;
        uint8_t temp_buffer[BUFF_SIZE];

        while (*should_continue) {

            // read into the global buffer
            int bytes_read = net::receive(server_conn, temp_buffer, BUFF_SIZE);


            // if there's an issue reading, do nothing
            // TODO: add appropriate handline if recv causes error, maybe break out of loop?
            // TODO: also allow for multiple message parses before it re-loops
            if (bytes_read != -1) {
                // append global buffer data to the end of the client_buffer
                client_buffer.insert(client_buffer.end(), temp_buffer, temp_buffer + bytes_read);

                // if the size of the buffer is enough to read the length, read it and check if we can parse a message
                if (client_buffer.size() >= 4) {

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
                        handle_message(msg, e_queue, lock);

                    }
                } 
            }
            

        }
    }

    void Client::parse_user_input(std::string line, AppState &app) {
        // TODO: for now, just force it to have one space after the /add before the username
        if (strncmp(line.c_str(), "/add", 4) == 0) {
            std::string user(line.c_str() + 5);
            std::cerr << "Attempting to add contact: " << user;
            std::vector<uint8_t> payload = serialize_message(
                serialize_req_user_id(user)
            );
            net::send_all(server_conn, payload.data(), payload.size());
        } else {
            if (app.selected_user >= app.uids.size()) {
                return;
            }
            std::cerr << "Attempting to send message to uid: " << app.uids[app.selected_user] << " from " << uid << "\n";
            std::vector<uint8_t> payload = serialize_message(
                serialize_chat(uid, app.uids[app.selected_user], line)
            );
            net::send_all(server_conn, payload.data(), payload.size());
        }
    }
}