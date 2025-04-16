#include "Packet.h"

RequestPacket::RequestPacket() : _version(0), _code(0), _payload_size(0), _uuid{ 0 } {}

void RequestPacket::setHeader(uint8_t* uuid, uint8_t version, uint16_t code, uint32_t payload_size) {
    _data.clear();
    _data.resize(HEADER_SIZE);
    std::memcpy(_data.data(), uuid, UUID_LEN);

    _data[UUID_LEN] = version;

    boost::endian::native_to_little_inplace(code);
    std::memcpy(_data.data() + UUID_LEN + 1, &code, 2);

    boost::endian::native_to_little_inplace(payload_size);
    std::memcpy(_data.data() + UUID_LEN + 3, &payload_size, 4);
}

void RequestPacket::appendPayload(const std::vector<uint8_t>&payload) {
    _data.insert(_data.end(), payload.begin(), payload.end());
}

void RequestPacket::appendPayload(const std::string payload) {
    (*this).appendPayload(std::vector<uint8_t>(payload.begin(), payload.end()));
}

void RequestPacket::appendPayload(const uint8_t* payload, size_t size) {
    (*this).appendPayload(std::vector<uint8_t>(payload, payload + size));
}

void RequestPacket::appendPayload(const char* payload, size_t size) {
    (*this).appendPayload(std::vector<uint8_t>(payload, payload + size));
}

void RequestPacket::appendMessageHeader(uint8_t* uuid, uint8_t messageType, uint32_t contentSize) {
    _data.resize(HEADER_SIZE + OUT_MESSAGE_HEADER_SIZE);
    std::memcpy(_data.data() + HEADER_SIZE, uuid, UUID_LEN);

    _data[HEADER_SIZE + UUID_LEN] = messageType;

    boost::endian::native_to_little_inplace(contentSize);
    std::memcpy(_data.data() + HEADER_SIZE + UUID_LEN + 1, &contentSize, 4);
}

void RequestPacket::send(boost::asio::ip::tcp::socket& socket) const {
    boost::asio::write(socket, boost::asio::buffer(_data));
}

ResponsePacket::~ResponsePacket() {
    delete _payload;
}

ResponsePacket* ResponsePacket::receive(boost::asio::ip::tcp::socket& socket) {
    //get the packet object with an header
    ResponsePacket* packet = receiveHeader(socket);
    //add a pointer to a payload vector and save the payload in the vector
    packet->_payload = new std::vector<uint8_t>(packet->_payload_size);
    boost::asio::read(socket, boost::asio::buffer(*(packet->_payload)));

    return packet;
}

ResponsePacket* ResponsePacket::receiveHeader(boost::asio::ip::tcp::socket& socket) {
    std::vector<uint8_t> header_buffer(RESPONSE_HEADER_SIZE);
    boost::asio::read(socket, boost::asio::buffer(header_buffer));
    ResponsePacket* packet = new ResponsePacket();
    packet->_version = header_buffer[0];
    std::memcpy(&(packet->_code), header_buffer.data() + 1, 2);
    boost::endian::little_to_native_inplace(packet->_code);
    std::memcpy(&(packet->_payload_size), header_buffer.data() + 3, 4);
    boost::endian::little_to_native_inplace(packet->_payload_size);
    packet->_payload = NULL;

    return packet;
}

uint8_t ResponsePacket::getVersion() {
    return _version;
}
uint16_t ResponsePacket::getCode() {
    return _code;
}
uint32_t ResponsePacket::getPayloadSize() {
    return _payload_size;
}
std::vector<uint8_t>* ResponsePacket::getPayload() {
    return _payload;
}