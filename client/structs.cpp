#include "structs.h"
#include <boost/endian/conversion.hpp>
#include <iostream>


//s_user_name methods

s_user_name::s_user_name(const s_user_name& other) {
	for (int i = 0; i < CLIENT_NAME_SIZE; i++) {
		user_name[i] = other.user_name[i];
	}
}

s_user_name::s_user_name(char* arr): user_name{ DEF_VAL } {
	for (int i = 0; i < CLIENT_NAME_SIZE && arr[i] != '\0'; i++)
		user_name[i] = arr[i];
}

s_user_name& s_user_name::operator=(const s_user_name& other) {
	for (int i = 0; i < CLIENT_NAME_SIZE; i++) {
		user_name[i] = other.user_name[i];
	}
	return *this;
}

bool s_user_name::operator==(const s_user_name& other) const {
	for (int i = 0; i < CLIENT_NAME_SIZE; i++) {
		if (user_name[i] != other.user_name[i])
			return false;
		if (user_name[i] == '\0')	//both usernames end here
			break;
	}
	return true;
}

bool s_user_name::operator!=(const s_user_name& other) const {
	return !(*this == other);
}

//s_uuid methods

s_uuid::s_uuid(const s_uuid& other) {
	for (int i = 0; i < UUID_LEN; i++) {
		uuid[i] = other.uuid[i];
	}
}

s_uuid::s_uuid(uint8_t* arr) {
	for (int i = 0; i < UUID_LEN; i++)
		uuid[i] = arr[i];
}

s_uuid& s_uuid::operator=(const s_uuid& other) {
	for (int i = 0; i < UUID_LEN; i++) {
		uuid[i] = other.uuid[i];
	}
	return *this;
}

bool s_uuid::operator==(const s_uuid& other) const {
	for (int i = 0; i < UUID_LEN; i++) {
		if (uuid[i] != other.uuid[i])
			return false;
	}
	return true;
}

bool s_uuid::operator!=(const s_uuid& other) const {
	return !(*this == other);
}

//s_client methods
s_client::s_client(s_uuid arg_uuid, s_user_name arg_user_name): symKey { 0 }, publicKey { 0 } {
	uuid = arg_uuid;
	userName = arg_user_name;
	publicKeySet = false;
	symKeySet = false;
	requestedSymKey = false;
}


//s_message methods
s_message::s_message(): uuid() {
	messageID = 0;
	messageType = 0;
	messageSize = 0;
	content = NULL;
}

s_message::s_message(std::vector<uint8_t>& header_buffer): uuid(), messageID(0), messageSize(0) {
	if (header_buffer.size() != OUT_MESSAGE_HEADER_SIZE && header_buffer.size() != IN_MESSAGE_HEADER_SIZE)
		throw (header_buffer);//a wrong size of header buffer given as parameter
	std::memcpy(uuid.uuid, header_buffer.data(), UUID_LEN);
	size_t messageIDFieldLen = 0; //We'll later be able to add it as the offset to the buffer
	if (header_buffer.size() == IN_MESSAGE_HEADER_SIZE) {
		messageIDFieldLen = 4;
		std::memcpy(&messageID, header_buffer.data() + UUID_LEN, messageIDFieldLen);
		boost::endian::little_to_native_inplace(messageID);
	}
	messageType = header_buffer[UUID_LEN + messageIDFieldLen];
	std::memcpy(&messageSize, header_buffer.data() + UUID_LEN + messageIDFieldLen + 1, 4); //1 for the size of messageType and 4 for the size of its size
	boost::endian::little_to_native_inplace(messageSize);
	content = new char[messageSize];
}

s_message::~s_message() {
	delete[] content;
}

s_message* read_message(boost::asio::ip::tcp::socket& socket) {
	std::vector<uint8_t> header_buffer(IN_MESSAGE_HEADER_SIZE);
	boost::asio::read(socket, boost::asio::buffer(header_buffer));
	s_message* msg = new s_message(header_buffer);
	if (msg->messageSize > 0) {
		boost::asio::read(socket, boost::asio::buffer(msg->content, msg->messageSize));
	}
	return msg;
}