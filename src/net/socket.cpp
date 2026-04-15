#include "net/socket.hpp"
#include <iostream>
#include <cstring>

namespace net {

    // initialize program for working with sockets, really only necessary in Windows
    void init_sockets() {
        #ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
        #endif
    }

    // clean up program after working with sockets, really only necessary in Windows
    void cleanup_sockets() {
        #ifdef _WIN32
        WSACleanup();
        #endif
    }


    // create a tcp socket, returns type of socket_t
    socket_t create_tcp_socket() {
        return ::socket(AF_INET, SOCK_STREAM, 0);
    }

    // closes a socket, on either Windows or POSIX systems
    void close_socket(socket_t s) {
        #ifdef _WIN32
        closesocket(s);
        #else
        close(s);
        #endif
    }

    // sets a socket up for non blocking io returns true if successful, false otherwise
    bool set_non_blocking(socket_t s) {
        #ifdef _WIN32
        u_long mode = 1;
        return ioctlsocket(s, FIONBIO, &mode) == 0;
        # else 
        int flags = fcntl(s, F_GETFL, 0);
        return fcntl(s, F_SETFL, flags | O_NONBLOCK) != -1;
        #endif
    }

    // binds a socket to a port
    bool bind_socket(socket_t s, uint16_t port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        return ::bind(s, (sockaddr *) &addr, sizeof(addr)) == 0;
    }

    // begin listening on a port
    bool listen_socket(socket_t s, int backlog) {
        return ::listen(s, backlog) == 0;
    }

    // accept a connection, returns the new connected socket
    socket_t accept_socket(socket_t server_fd) {
        sockaddr_in client{};

        #ifdef _WIN32
        int len = sizeof(client);
        #else
        socklen_t len = sizeof(client);
        #endif
        return ::accept(server_fd, (sockaddr*)&client, &len);
    }

    // attempt to connect to a host:port address returning the socket of the connection
    socket_t connect_to_host(std::string host, std::string port){
        socket_t sfd;
        int s;
        //char buf[128];
        socklen_t peer_addrlen;
        struct addrinfo hints;
        struct addrinfo *result = NULL, *rp = NULL;
        struct sockaddr_storage peer_addr;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = 0;
        hints.ai_protocol = 0;

        s = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);

        /* iterate through each returned address
        until we have a successful connection */
        for (rp = result; rp != NULL; rp = rp->ai_next) {

            /* attempt a creation of a socket */
            sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

            /* if socket creation failed, move to the next address */
            if (sfd == INVALID_SOCKET) {
                continue;
            }

            /* attempt to connect using the socket, if successful return socket */
            if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
                freeaddrinfo(result);
                return sfd;
            }

            /* if the connection wasn't successful, close the socket and move on */
            close_socket(sfd);
        }

        freeaddrinfo(result);
        return INVALID_SOCKET;
    }

    // attempts to send all the data on a connection, returns the number of bytes sent, returning -1 on error
    int send_all(socket_t s, const uint8_t *data, size_t len) {
        size_t total = 0;
        while (total < len) {
            int sent = ::send(s, (const char *) data + total, len - total, 0);
            if (sent <= 0) {
                return total;
            }
            total += sent;
        }

        return (int) total;
    }


    // receive a specified number of bytes on a socket
    int receive(socket_t s, uint8_t *buffer, size_t len) {
        return ::recv(s, (char *) buffer, len, 0);
    }
}

