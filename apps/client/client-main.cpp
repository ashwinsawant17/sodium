#include "net/socket.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>

#define BUFF_SIZE 128

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

    net::set_non_blocking(conn);

    std::string username;
    std::cout << "Enter your username:\n";
    std::cin >> username;

    // for now, begin a read loop, and just send all the bytes to the server
    do {
        std::vector<uint8_t> buffer = {'t', 'e', 's', 't'};
        buffer.reserve(BUFF_SIZE);
        int bytes_read = 0;
        bytes_read = net::receive(conn, buffer.data(), buffer.size());

        std::string str(buffer.begin(), buffer.end());

        if (bytes_read > 0) {
            std::cout << str;
            std::cout << "\n";
        }

        std::cin >> input;
        std::string data = username + ": " + input;


        if (input != "exit") {
            std::cou
            // TODO: add check for how many bytes are actually sent
            net::send_all(
                conn, 
                reinterpret_cast<const uint8_t *>(data.c_str()), 
                data.length()
            );
        }
    } while (input != "exit");

    net::cleanup_sockets();

    return 0;
}