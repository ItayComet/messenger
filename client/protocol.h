#ifndef PROTOCOL_H
#define PROTOCOL_H

enum class Req {
	REGISTER = 600,
	CLIENT_LIST = 601,
	PUBLIC_KEY = 602,
	SEND_MESSAGE = 603,
	PULL_MESSAGE = 604
};

enum class Response {
	REGISTER_SUCCESS = 2100,
	CLIENT_LIST = 2101,
	PUBLIC_KEY = 2102,
	MESSAGE_SENT = 2103,
	WAITING_MESSAGES = 2104,
	G_ERROR = 9000
};

enum class MessageType {
	SYM_KEY_REQ = 1,
	SYM_KEY_SENT = 2,
	TEXT = 3
};

#endif