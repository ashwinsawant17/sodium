#include "net/server.hpp"
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

    // intialize the server with the correct port number
    net::Server server = net::Server(PORT);

    // start the server
    server.start();



    net::cleanup_sockets();
    return 0;
}