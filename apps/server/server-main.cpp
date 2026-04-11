#include "net/socket.hpp"
#include <iostream>

int main(void) {
    net::init_sockets();
    std::cout << "Hello from server.\n";

    net::cleanup_sockets();
    return 0;
}