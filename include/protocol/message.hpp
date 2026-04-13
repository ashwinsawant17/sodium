#pragma once

#include <cstdint>
#include <vector>

namespace protocol {


    // Type of Message sent to server
    //
    // System: a system message ex: settings update, server updates, etc.
    //
    // Chat: a message between two users
    enum class MessageType :  uint8_t {
        System = 1,
        Chat = 2
    };

    // Message Struct
    struct Message {
        MessageType type;
        std::vector<uint8_t> payload;
    };

    // Chat Payload
    struct ChatPayload {
        
    }



}