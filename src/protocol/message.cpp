#include "protocol/message.hpp"
#include <stdexcept>
#include <string>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

namespace protocol {

    // TODO: UPDATE SOON TO BE TLV (TYPE-LENGTH-VALUE) TO ALLOW MESSAGE SKIPPING
    // messages should be serialized as such
    // [length (4 bytes)][type of payload (1 byte)][payload]

    // type of payloads
    // auth:
    // [length of username (2 byte)][username (<length of username> bytes)]

    // chat:
    // [length of message (2 bytes)][message (<length of message> bytes)]
    // for now, server will define the messages by the socket it comes in on
    
    // serialize an auth message into a Message Struct
    Message serialize_auth(std::string username) {
        // TODO: how to handle little vs big endian discrepancies?
        uint16_t chat_len = username.length();

        // use htons to transform into network endianness
        chat_len = htons(chat_len);

        std::vector<uint8_t> out;
        out.reserve(sizeof(chat_len) + chat_len);

        out.push_back((chat_len >> 8) & 0xFF);
        out.push_back((chat_len) & 0xFF);

        out.insert(out.end(), username.begin(), username.end());

        MessageType type = MessageType::Auth;

        Message output {type, out};

        return output;
    }

    // serialize a chat message into a Message Struct
    Message serialize_chat(std::string chat_message) {
        
        // TODO: how to handle little vs big endian discrepancies?
        uint16_t chat_len = chat_message.length();

        // use htons to transform into network endianness
        chat_len = htons(chat_len);

        std::vector<uint8_t> out;
        out.reserve(sizeof(chat_len) + chat_len);

        out.push_back((chat_len >> 8) & 0xFF);
        out.push_back((chat_len) & 0xFF);

        out.insert(out.end(), chat_message.begin(), chat_message.end());

        MessageType type = MessageType::Chat;

        Message output {type, out};

        return output;
    }
    
    // deserialize a Message Struct with type of Chat into a chat string
    std::string deserialize_chat(Message message) {
        // validate the message type
        if (message.type != MessageType::Chat) {
            throw std::invalid_argument("Received Message with non Chat Type");
        
        // in the case of valid input, deserialize the payload
        } else {

            // get the length of the chat message
            uint16_t chat_len = 0;
            chat_len = (message.payload[0] << 8) & message.payload[1];

            // correct endianness of chat length
            chat_len = ntohs(chat_len);

            // lowkey not sure if I did this right, should i just use payload.end()?
            std::string output(message.payload.begin() + 2, message.payload.begin() + 2 + chat_len);
            // TODO: why are we even encoding chat length if we encode overall payload length?
            // I guess it makes it more scalable in case we want to include more nested structures in the payload?
            
            return output;
        }
    }

    // deserialize a Message Struct with type of Chat into a chat string
    std::string deserialize_auth(Message message) {
        // validate the message type
        if (message.type != MessageType::Auth) {
            throw std::invalid_argument("Received Message with non Chat Type");
        
        // in the case of valid input, deserialize the payload
        } else {

            // get the length of the chat message
            uint16_t chat_len = 0;
            chat_len = (message.payload[0] << 8) & message.payload[1];

            // correct endianness of chat length
            chat_len = ntohs(chat_len);

            // lowkey not sure if I did this right, should i just use payload.end()?
            std::string output(message.payload.begin() + 2, message.payload.begin() + 2 + chat_len);
            // TODO: why are we even encoding chat length if we encode overall payload length?
            // I guess it makes it more scalable in case we want to include more nested structures in the payload?
            
            return output;
        }
    }

    // serialize a message struct into a byte buffer
    std::vector<uint8_t> serialize_message(Message message) {

        // get the length of the message
        // include the type in the length, because we are using ltv NOT TLV TODO: SEE TOP OF MESSAGE.CPP FILE
        uint32_t chat_len = sizeof(message.type);

        // technically .size() returns number of elements, not overall byte size, but since each element is 1 byte, should be find
        chat_len += message.payload.size();
        
        // convert to network endianness
        // make sure to use "long" versions of hton or ntoh
        chat_len = htonl(chat_len);

        // output vector, reserve the size needed for the message
        std::vector<uint8_t> out;
        out.reserve(sizeof(chat_len) + sizeof(message.type) + chat_len);

        // push the message type
        out.push_back(static_cast<uint8_t>(message.type));


        // push the chat length
        out.push_back(chat_len & 0xFF);
        out.push_back((chat_len >> 8) & 0xFF);
        out.push_back((chat_len >> 16) & 0xFF);
        out.push_back((chat_len >> 24) & 0xFF);

        
        // append the payload
        out.insert(out.end(), message.payload.begin(), message.payload.end());

        return out;
    }

    // deserialize a buffer into a message struct (assumes length has already been removed)
    Message deserialize_message(std::vector<uint8_t> buffer) {
        std::vector<uint8_t> message(buffer.begin() + 1, buffer.end());
        MessageType type = static_cast<MessageType>(buffer[0]);

        return Message{type, message};
    }


}