#include "net/socket.hpp"
#include <iostream>
#include <stdexcept>

int main(void) {
    net::init_sockets();
    std::cout << "Hello from client.\n";

    // get the host:port from stdin
    std::string input;
    std::cout << "Enter the HOST:PORT\n";
    std::cin >> input;

    size_t pos = input.find(":");

    if (pos == std::string::npos) {
        throw std::runtime_error("Invalid format for host:port.\n");
    }

    // separate the host from the port
    std::string host = input.substr(0, pos);
    std::string port = input.substr(pos + 1);

    // attempt to connect to the host:port
    net::socket_t conn = net::connect_to_host(host, port);
    if (conn == INVALID_SOCKET) {
        throw std::runtime_error("Connecting to host failed.\n");
    }

    // for now, begin a read loop, and just send all the bytes to the server
    do {
        std::cin >> input;
        if (input != "exit") {
            // TODO: add check for how many bytes are actually sent
            net::send_all(
                conn, 
                reinterpret_cast<const uint8_t *>(input.c_str()), 
                input.length()
            );
        }
    } while (input != "exit");

    net::cleanup_sockets();

    return 0;
}