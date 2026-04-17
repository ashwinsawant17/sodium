#pragma once

#include "net/socket.hpp"
#include "protocol/message.hpp"

#include <unordered_set>
#include <unordered_map>

namespace net {

    #define SERVER_BUFFER_SIZE 1024

    class Server {
        private:
            net::socket_t server_socket;
            uint16_t port;

            // set of all connected clients
            std::unordered_set<socket_t> clients;

            // map from connections to buffers
            std::unordered_map<socket_t, std::vector<uint8_t>> client_buffers;
            
            // TODO: switch to a UID model for user management, usernames should just be for auth/vanity
            // 
            // map from connections to usernames
            std::unordered_map<socket_t, std::string> client_usernames;

            // global buffer for reading
            uint8_t global_buffer[SERVER_BUFFER_SIZE];

        public:
            Server(uint16_t port);
            ~Server();

            void handle_message(socket_t conn, protocol::Message msg);

            // use the connection and buffer to correctly parse incoming bytes
            //
            // calls correct handler based on parsed message type
            // 
            // returns False if more bytes need to be read to be parsed, or if it can't be parsed.
            //
            // returns True if parsed correctly
            bool buffered_read(socket_t conn, std::vector<uint8_t>& buffer);

            void start();

    };


}