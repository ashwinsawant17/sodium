#include "protocol/message.hpp"
#include <string>

namespace protocol {

    // messages should be serialized as such
    // [length (4 bytes)][type of payload (1 byte)][payload]

    // type of payloads
    // auth:
    // [length of username (1 byte)][username (<length of username> bytes)]

    // chat:
    // [length of message (2 bytes)][message (<length of message> bytes)]
    // for now, server will define the messages by the socket it comes in on
    

    Message serialize_chat(std::string chat_message) {
        
        // TODO: how to handle little vs big endian discrepancies?
        uint16_t chat_len = chat_message.length();

        std::vector<uint8_t> out;
        out.reserve(sizeof(chat_len) + chat_len);

        out.push_back((chat_len >> 8) & 0xFF);
        out.push_back((chat_len) & 0xFF);

        out.insert(out.end(), chat_message.begin(), chat_message.end());

        MessageType type = MessageType::Chat;

        Message output {type, out};

        return output;
    }


}