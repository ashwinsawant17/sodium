#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace protocol {


    // Type of Message sent to server
    //
    // System: a system message ex: settings update, server updates, etc.
    //
    // Chat: a message between two users
    enum class MessageType :  uint8_t {
        System = 1,
        Chat = 2,
        Auth = 3
    };

    // Message Struct
    struct Message {
        MessageType type;
        std::vector<uint8_t> payload;
    };

    // serialize an auth message into a Message Struct
    Message serialize_auth(std::string username);

    // serialize a chat message into a Message Struct
    Message serialize_chat(std::string chat_message);

    // deserialize a Message Struct with type of Chat into a chat string
    std::string deserialize_chat(Message message);

    // deserialize a Message Struct with type of Chat into a chat string
    std::string deserialize_auth(Message message);

    // serialize a message struct into a byte buffer
    std::vector<uint8_t> serialize_message(Message message);

    // deserialize a buffer into a message struct (assumes length has already been removed)
    Message deserialize_message(std::vector<uint8_t> buffer);


}