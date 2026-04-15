#include "net/socket.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

#define BUFF_SIZE 128

void listen_print(net::socket_t s, bool* should_continue) {
    while (*should_continue) {
        std::vector<uint8_t> buffer;
        buffer.resize(BUFF_SIZE);
        int bytes_read = 0;
        bytes_read = net::receive(s, buffer.data(), buffer.size());

        std::string str(buffer.begin(), buffer.end());

        if (bytes_read > 0) {
            std::cout << str;
            std::cout << "\n";
        }
    }
}

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

    bool non_block_success = net::set_non_blocking(conn);
    if (non_block_success) {
        std::cout << "Setting non-blocking: SUCCESS\n";
    } else {
        std::cout << "Setting non-blocking: FAIL\n";
    }

    std::string username;
    std::cout << "Enter your username:\n";
    std::cin >> username;

    // for now, begin a read loop, and just send all the bytes to the server
    bool should_continue = true;
    std::thread receiver_thread(listen_print, conn, &should_continue); 
    do {

        std::cin >> input;
        std::string data = username + ": " + input;


        if (input != "exit") {
            std::cout << "Input received.\n";
            // TODO: add check for how many bytes are actually sent
            net::send_all(
                conn, 
                reinterpret_cast<const uint8_t *>(data.c_str()), 
                data.length()
            );
        }
    } while (input != "exit");

    should_continue = false;
    receiver_thread.join();
    net::cleanup_sockets();

    return 0;
}