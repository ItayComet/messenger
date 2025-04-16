#ifndef PACKET_H
#define PACKET_H

#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <cstdint>
#include <cstring>
#include "structs.h"

/*
* The class is holds and sends a request to the server, it has multiple utility functions to make sending a request easier
*/
class RequestPacket {
private:
    std::vector<uint8_t> _data;
    char _uuid[UUID_LEN];
    uint8_t _version;
    uint16_t _code;
    uint32_t _payload_size;
    std::vector<std::string> _clients_list;
    std::vector<std::string> _waiting_messages;
    std::vector<uint8_t> _client_id;
    std::vector<uint8_t> _public_key;
    std::vector<uint8_t> _message_ack;

public:
    //creates an empty packet
    RequestPacket();

    //sets the header according to the arguments
    void setHeader(uint8_t* uuid, uint8_t version, uint16_t code, uint32_t payload_size);

    //multiple overides so I'll be able to append payload from multiple types of objects
    void appendPayload(const std::vector<uint8_t>& payload);
    void appendPayload(const uint8_t* payload, size_t size);
    void appendPayload(const char* payload, size_t size);
    void appendPayload(const std::string payload);

    //add an header for a message (it's still a part of the payload, but it's an easier way to send messages)
    void appendMessageHeader(uint8_t* uuid, uint8_t messageType, uint32_t contentSize);

    //sends the packet through the socket
    void send(boost::asio::ip::tcp::socket& socket) const;
};

/*
* This class reads responses from the the host
*/
class ResponsePacket {
private:
    uint8_t _version;
    uint16_t _code;
    uint32_t _payload_size;
    std::vector<uint8_t> *_payload;
    
public:
    ResponsePacket(): _version(0), _code(0), _payload_size(0) {}

    ~ResponsePacket();

    //reads a full packet from the socket and stores it as ResponsePacket object
    static ResponsePacket* receive(boost::asio::ip::tcp::socket& socket);

    //We'll use it mainly for pull messages request, so we can read the messages one by one
    static ResponsePacket* receiveHeader(boost::asio::ip::tcp::socket& socket);

    //getters
    uint8_t getVersion();
    uint16_t getCode();
    uint32_t getPayloadSize();
    std::vector<uint8_t>* getPayload(); //This one returns a pointer to the payload saved in the class in order to save up on space

};

#endif // PACKET_H
