#ifndef CLIENT_H
#define CLIENT_H

#include <boost/asio.hpp>
#include <iomanip>
#include "packet.h"
#include "structs.h"
#include "RSAWrapper.h"
#include "AESWrapper.h"
#include "protocol.h"
#include "Base64Wrapper.h"
using boost::asio::ip::tcp;

#define USER_INFO_FILE "me.info"
#define VERSION 1

enum Menu {
	REGISTER = 110,
	CLIENT_LIST = 120,
	PUBLIC_KEY = 130,
	PULL_MESSAGE = 140,
	SEND_MESSAGE = 150,
	REQ_SYM_KEY = 151,
	SEND_SYM_KEY = 152,
	SEND_FILE = 153,
	EXIT = 0
};

/*
* This class handles inputs from the client and sends packets to the host as instructed in protocol
* To 
*/
class Client {
private:
	boost::asio::io_context _ioContext;
	tcp::socket _socket;
	tcp::resolver _resolver;
	s_client _self;
	std::vector<s_client*> _clients;
	RSAPrivateWrapper* _RSAWrapper;
	bool registered;
	bool readInfoFile();
	bool writeInfoFile();
	int findClient(s_user_name user_name);
	int findClient(s_uuid id);
public:
	/*
	* A constructor to the client, accept ip and port as arguements, and connects to the server.
	* if exists and written in a valid way, a user info file will be read and its contents saved,
	* then the user will be considered registered, else, they will have to register to access any function.
	*/
	Client(const std::string& ip, const std::string& port);
	/* A simple destructor*/
	~Client();
	/*
	* Once a client object had been constructed, the run method runs the main loop of the client
	* As long as the program keeps running (unless the user has chosen to exit through the menu), the loop
	* will prompt the user for the next command, and will run the appropriate handle method.
	* If the user is not registered, all options other than register and exit will be blocked.
	*/
	void run();

	//Each method handles a request from the user
	void handleRegReq();
	void handleClientListReq();
	void handlePublicKeyReq();
	void handlePullMessagesReq();
	void handleSendMessage();
	void handleSymKeyReq();
	void handleSendSymKey();
};

#endif