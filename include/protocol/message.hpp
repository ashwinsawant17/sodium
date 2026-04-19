#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace protocol {

    // type for a user id
    using uid_t = uint32_t;
    #define INVALID_UID 0

    // struct for a chat message, will define later protocol
    struct Direct_Message {
        uid_t sender;
        uid_t receiver;
        std::string chat;
    };

    // Type of Message sent to server
    //
    // System: a system message ex: settings update, server updates, etc.
    //
    // Chat: a message between two users
    enum class MessageType :  uint8_t {
        System = 1,
        Chat = 2,
        Auth = 3,
        REQ_USER_ID = 4,
        REQ_USERNAME = 5,
        USER_INFO = 6
    };

    // Message Struct
    struct Message {
        MessageType type;
        std::vector<uint8_t> payload;
    };

    // serialize a request for a user id from a username
    Message serialize_req_user_id(std::string username);

    // deserialize a request for a user id from a username;
    std::string deserialize_req_user_id(Message msg);

    // serialize a request for a username from user id
    Message serialize_req_username(uid_t uid);

    // deserialize a request for a username from user id
    uid_t deserialize_req_username(Message msg);

    // serialize user info
    Message serialize_user_info(uid_t uid, std::string username);

    // deserialize user info
    std::pair<uid_t, std::string> deserialize_user_info(Message msg);

    // serialize an auth message into a Message Struct
    Message serialize_auth(std::string username);

    // serialize a chat message into a Message Struct
    Message serialize_chat(uid_t sender, uid_t receiver, std::string chat_message);

    // deserialize a Message Struct with type of Chat into a chat string
    std::tuple<uid_t, uid_t, std::string> deserialize_chat(Message message);

    // deserialize a Message Struct with type of Chat into a chat string
    std::string deserialize_auth(Message message);

    // serialize a message struct into a byte buffer
    std::vector<uint8_t> serialize_message(Message message);

    // deserialize a buffer into a message struct (assumes length has already been removed)
    Message deserialize_message(uint32_t length, std::vector<uint8_t> buffer);


}