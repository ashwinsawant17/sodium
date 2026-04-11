#pragma once

#include <string>
#include <vector>
#include <cstdint>


#ifdef _WIN32
    #define NOMINMAX
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using socket_t = SOCKET;
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    using socket_t = int;
#endif


namespace net {
    // initialize program for working with sockets, really only necessary in Windows
    void init_sockets();

    // clean up program after working with sockets, really only necessary in Windows
    void cleanup_sockets();

    // create a tcp socket, returns type of socket_t
    socket_t create_tcp_socket();

    // closes a socket, on either Windows or POSIX systems
    void close_socket(socket_t s);

    // sets a socket up for non blocking io returns true if successful, false otherwise
    bool set_non_blocking(socket_t s);

    // binds a socket to a port
    bool bind_socket(socket_t s, uint16_t port);

    // begin listening on a port
    bool listen_socket(socket_t s, int backlog = 128);

    // accept a connection, returns the new connected socket
    socket_t accept_socket(socket_t server_fd);

    // attempts to send all the data on a connection, returns the number of bytes sent, returning -1 on error
    int send_all(socket_t s, const uint8_t *data, size_t len);

    // receive a specified number of bytes on a socket
    int recv_some(socket_t s, uint8_t *buffer, size_t len);

}