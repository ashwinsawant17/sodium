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
        return fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
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

    // attempts to send all the data on a connection, returns the number of bytes sent, returning -1 on error
    int send_all(socket_t s, const uint8_t *data, size_t len) {
        size_t total = 0;
        while (total < len) {
            int sent = ::send(s, (const char *) data + total, len - total, 0);
            if (sent <= 0) {
                return sent;
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

