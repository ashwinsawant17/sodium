#pragma once

#include "net/socket.hpp"
#include <unordered_set>
#include <unordered_map>

namespace net {

    class Server {
        private:
            net::socket_t server_socket;
            uint16_t port;

            std::unordered_set<socket_t> clients;
        public:
            Server(uint16_t port);
            ~Server();

            void start();

    };


}