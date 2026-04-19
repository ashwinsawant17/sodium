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
            // note, it doesn't map from uid to buffer, because buffers need to be per connected client, not per user
            std::unordered_map<socket_t, std::vector<uint8_t>> client_buffers;
            
            // TODO: switch to a UID model for user management, usernames should just be for auth/vanity
            // 
            // map from connections to uids
            std::unordered_map<socket_t, uid_t> client_uids;

            // map from uid to username (username just for vanity)
            std::unordered_map<uid_t, std::string> client_usernames;

            // global buffer for reading
            uint8_t global_buffer[SERVER_BUFFER_SIZE];

            // last uuid generated
            uid_t last_uid;

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

            // generates a new uid, and sets the last_uid variable to the generated value
            uid_t gen_uid();


            void start();

    };


}