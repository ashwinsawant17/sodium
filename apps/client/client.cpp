#include "net/client.hpp"
#include "net/socket.hpp"

namespace net {


    Client::Client(std::string host, std::string port, std::string username) {
        init_sockets();


        // connect to the server
        server_conn = connect_to_host(host, port);
        if (server_conn == INVALID_SOCKET) {
            throw std::runtime_error("Connecting to host failed.\n");
        }

        // set the connection to be nonblocking
        bool non_block_success = set_non_blocking(server_conn);
        if (!non_block_success) {
            throw std::runtime_error("Could not set socket to non-blocking.\n");
        }

        // send an auth message
        // TODO: probably need some sort of confirmation/ UID generated
        std::vector<uint8_t> auth_message = serialize_message(
            serialize_auth(username)
        );
        send_all(server_conn, auth_message.data(), auth_message.size());
    }

    Client::~Client() {
        cleanup_sockets();
    }
}