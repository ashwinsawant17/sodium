#include "net/socket.hpp"
#include <iostream>
#include <cstring>

/* library for using epoll */
#include <sys/epoll.h>
#define MAX_EVENTS 10
#define BUFF_SIZE 1024



int main(int argc, char const *argv[]) {

    const uint16_t PORT = static_cast<uint16_t>(std::stoi(argv[1]));

    net::init_sockets();
    std::cout << "Hello from server.\n";

    // Initialize server socket
    net::socket_t server_socket = net::create_tcp_socket();

    // bind the server socket to the port
    net::bind_socket(server_socket, PORT);

    // begin listening on the socket for new connections
    net::listen_socket(server_socket);

    std::cout << "Listening on PORT: " << PORT << "\n";

    // use epoll to handle all sockets and connections
    // add each socket to an epoll instance
    // wait for the epoll to recognize new info to read
    // if it's the server socket that made epoll register, it's a new connection
    // register the conneection, and add it to the epoll instance
    // if the connection socket was the one that epoll reacted to, then begin reading/writing to appropriate output


    // create the epoll instance
    int epoll_fd = epoll_create1(0);

    // dedicated epoll event for registering new fd's
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_socket;

    // register the server socket onto the epoll instance
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ev);


    // array of epoll events for accepted connections
    struct epoll_event events[MAX_EVENTS];

    // read from connection and for now, print out received message
    uint8_t buffer[BUFF_SIZE] = {0 };

    // for now get a listen loop going
    while(true) {
        int nfds, conn_sock;

        // wait for epoll events and get the number of returned fds in nfds
        nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        // iterate through each event and handle appropriately
        for (int i = 0; i < nfds; i++) {

            // if returned fd is listening socket, this means there's a new connection to be accepted
            if (events[i].data.fd == server_socket) {

                // accept connection
                net::socket_t conn = net::accept_socket(server_socket);

                // set the connection as nonblocking
                net::set_non_blocking(conn);

                // register the connection fd to epoll instance
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn, &ev);
            
            // if event is some other connection, read from connection and for now, simply write it to stdout
            } else {

                int bytes_read {0};
                
                do {
                    bytes_read = net::receive(events[i].data.fd, buffer, BUFF_SIZE - 1);
                    std::cout << buffer;
                } while (bytes_read > 0);
                std::cout << "\n";


            }

        }
    }



    net::cleanup_sockets();
    return 0;
}