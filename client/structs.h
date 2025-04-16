#ifndef STRUCTS_H
#define STRUCTS_H

/*
* This header holds structs which wrap important data and allows comparing some
*/
#include <vector>
#include <string>
#include <stdexcept>
#include <boost/asio.hpp>

#define UUID_LEN 16
#define DEF_VAL 0
#define CLIENT_NAME_SIZE 255
#define SYM_KEY_SIZE 16
#define PUBLIC_KEY_SIZE 160
#define HEADER_SIZE 23
#define RESPONSE_HEADER_SIZE 7
#define OUT_MESSAGE_HEADER_SIZE 5 + UUID_LEN //Message size + message type == 5 bytes
#define IN_MESSAGE_HEADER_SIZE OUT_MESSAGE_HEADER_SIZE + 4 //this one includes the message id
#define SOM_STRING "---------------"
#define EOM_STRING "-----<EOM>-----"
#define RSA_ENC_SIZE 128

/*
* A wrapper struct for the user name.
* Its only member variable is an array in the size of the user name
* An operator== is implemented for comparing.
*/
struct s_user_name {
	char user_name[CLIENT_NAME_SIZE];
	
	s_user_name() : user_name{ DEF_VAL } {};
	s_user_name(const s_user_name& other);
	s_user_name(char* arr);

	s_user_name& operator=(const s_user_name& other);
	bool operator==(const s_user_name& other) const;
	bool operator!=(const s_user_name& other) const;
};


/*
* A wrapper struct for the uuid
* Its only member variable is an array in the size of the uuid in bytes
*/
struct s_uuid {
	uint8_t uuid[UUID_LEN];

	//an empty constructor, initialises the uuid arrau with zeroes
	s_uuid(): uuid{DEF_VAL}{}
	s_uuid(const s_uuid& other);
	s_uuid(uint8_t* arr);

	s_uuid& operator=(const s_uuid& other);
	bool operator==(const s_uuid& other) const;
	bool operator!=(const s_uuid& other) const;
};

struct s_req_header {
	s_uuid uuid;
	uint8_t version;
	uint16_t code;
	uint32_t payloadSize;

};

/*
* A wrapper class for a client (any other client as well, not just the user)
*/
struct s_client {
	s_uuid uuid;
	s_user_name userName;
	char publicKey[PUBLIC_KEY_SIZE];
	bool publicKeySet;
	unsigned char symKey[SYM_KEY_SIZE];
	bool symKeySet;
	bool requestedSymKey;

	s_client() : publicKey{ 0 }, publicKeySet(false), symKey{ 0 }, symKeySet(false), requestedSymKey(false) {}
	s_client(s_uuid arg_uuid, s_user_name arg_user_name);
};

struct s_message {
	s_uuid uuid; //of the sender in an incoming message, or the reciever in an outgoing message
	uint32_t messageID;
	uint8_t messageType;
	uint32_t messageSize;
	char* content;

	//empty constructor
	s_message();

	//reads the buffer as it is defined in the protocol and saves the prameters in the right fields, allocates the char array of the content as well
	s_message(std::vector<uint8_t>& header_buffer);
	~s_message();
};

s_message* read_message(boost::asio::ip::tcp::socket& socket);

#endif //STRUCTS_H
