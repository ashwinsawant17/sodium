#include "net/socket.hpp"
#include "protocol/message.hpp"
#include "net/client.hpp"

using namespace protocol;
using namespace net;

#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

#define BUFF_SIZE 1024


int main(void) {
    // get the host:port from stdin
    std::string input;
    std::cout << "Enter the HOST:PORT\n";
    std::getline(std::cin, input);

    size_t pos = input.find(":");

    if (pos == std::string::npos) {
        throw std::runtime_error("Invalid format for host:port.\n");
    }

    // separate the host from the port
    std::string host = input.substr(0, pos);
    std::string port = input.substr(pos + 1);

    std::string username;
    std::cout << "Enter your username:\n";
    std::getline(std::cin, username);

    Client client = Client(host, port, username);

    // for now, begin a read loop, and just send all the bytes to the server
    bool should_continue = true;
    std::thread receiver_thread(&Client::listen, &client, &should_continue); 
    do {

        std::getline(std::cin, input);

        if (input != "/exit") {
            // TODO: add check for how many bytes are actually sent
            client.parse_user_input(input);
        }
    } while (input != "/exit");

    should_continue = false;
    receiver_thread.join();
    net::cleanup_sockets();

    return 0;
}