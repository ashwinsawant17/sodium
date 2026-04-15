#include "net/server.hpp"
#include "net/socket.hpp"
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
                    std::cout << "Newly registered connection: " << conn_sock << "\n";
                    auto [iter, success] = clients.insert(conn_sock);
                    if (success) {
                        std::cout << "Connection to " << conn_sock << " successful.\n";
                    } else {
                        std::cout << "Connection to " << conn_sock << " failed.\n";
                    }
                } else {
                    std::cout << "Received message from " << events[i].data.fd << "\n";

                    // TODO: for now, broadcast the message to all other 
                    socket_t sender_socket = events[i].data.fd;

                    int bytes_read { 0 };
                    std::vector<uint8_t> buffer;
                    buffer.resize(BUFF_SIZE);

                    bytes_read = receive(sender_socket, buffer.data(), BUFF_SIZE - 1);
                    
                    // bytes read was 0, the connection closed gracefully (or -1 if error, for now handle the same way)
                    // TODO: add separate handling for a gracefully closed connection vs an error
                    if (bytes_read <= 0) {
                        close_socket(sender_socket);
                        clients.erase(sender_socket);
                    } else {
                        std::string str(buffer.begin(), buffer.begin() + bytes_read);

                        for (const auto& client: clients) {
                            if (client != sender_socket) {
                                send_all(client, (const uint8_t *) str.c_str(), str.length());
                            }
                        }
                    }
                    
                    
                }
            }
        }
    }
}