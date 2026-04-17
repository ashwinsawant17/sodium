#include "net/socket.hpp"
#include "protocol/message.hpp"

using namespace protocol;


#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

#define BUFF_SIZE 1024

void handle_message(net::socket_t conn, protocol::Message msg) {
    if (msg.type == protocol::MessageType::Chat) {
        std::string output = deserialize_chat(msg);
        std::cout << output << "\n";
    }
}

void listen_print(net::socket_t conn, bool* should_continue) {
    std::vector<uint8_t> client_buffer;
    uint8_t temp_buffer[BUFF_SIZE];
    while (*should_continue) {

            // read into the global buffer
            int bytes_read = net::receive(conn, temp_buffer, BUFF_SIZE);


            // if there's an issue reading, do nothing
            // TODO: add appropriate handline if recv causes error, maybe break out of loop?
            // TODO: also allow for multiple message parses before it re-loops
            if (bytes_read != -1) {
                // append global buffer data to the end of the client_buffer
                client_buffer.insert(client_buffer.end(), temp_buffer, temp_buffer + bytes_read);

                // if the size of the buffer is enough to read the length, read it and check if we can parse a message
                if (client_buffer.size() >= 4) {

                    // read the message length
                    uint32_t message_len {0};
                    message_len |= (client_buffer[0] << 24);
                    message_len |= (client_buffer[1] << 16);
                    message_len |= (client_buffer[2] << 8);
                    message_len |= (client_buffer[3]);

                    // ensure that length has proper endianness
                    message_len = ntohl(message_len);

                    // if we can read the entire message, parse and handle it
                    if (client_buffer.size() >= 4 + message_len) {

                        // pop the length from the buffer
                        client_buffer.erase(client_buffer.begin(), client_buffer.begin() + 4);

                        // parse the message into a message struct
                        protocol::Message msg = protocol::deserialize_message(message_len, client_buffer);

                        // clear the message from the buffer
                        client_buffer.erase(client_buffer.begin(), client_buffer.begin() + message_len);

                        // handle the parsed message TODO: consider using a thread to handle this?
                        handle_message(conn, msg);

                    }
                } 
            }
            

        }
}

int main(void) {
    net::init_sockets();

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

    // send an auth message
    // TODO: probably need some sort of confirmation? maybe a new message type or a subset of a system message
    std::vector<uint8_t> auth_message = serialize_message(
        serialize_auth(username)
    );

    // TODO: check for how many bytes actually sent properly?
    net::send_all(conn, auth_message.data(), auth_message.size());



    // for now, begin a read loop, and just send all the bytes to the server
    bool should_continue = true;
    std::thread receiver_thread(listen_print, conn, &should_continue); 
    do {

        std::getline(std::cin, input);

        if (input != "exit") {
            // TODO: add check for how many bytes are actually sent
            std::vector<uint8_t> chat_message = serialize_message(
                serialize_chat(input)
            );
            net::send_all(conn, chat_message.data(), chat_message.size());
        }
    } while (input != "exit");

    should_continue = false;
    receiver_thread.join();
    net::cleanup_sockets();

    return 0;
}