#pragma once
#include "net/socket.hpp"
#include "protocol/message.hpp"
#include "client/appstate.hpp"

#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <map>

using namespace net;
using namespace protocol;

namespace net {
    class Client {
        public:
            // connection to the server
            socket_t server_conn;

            // client's username
            std::string username;

            // client's uid on the server
            uid_t uid;

            // client's mapping from uid's to usernames
            //
            // we use an *ordered* map so that we caan sort by the most recently message received
            std::unordered_map<uid_t, std::string> username_map;
        
        
            Client(std::string host, std::string port, std::string username);
            ~Client();
            void listen(bool *should_continue, std::queue<Event> &e_queue, std::mutex &lock);
            void handle_message(Message msg, std::queue<Event> &e_queue, std::mutex &lock);
            void parse_user_input(std::string line, AppState &app);
    };
}