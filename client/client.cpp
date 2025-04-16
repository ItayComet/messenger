#include "client.h"
#include "utils.h"

/*
* Utility function for the constructor
*/
bool Client::readInfoFile() {
    std::ifstream file(USER_INFO_FILE);

    // If the file doesn't exist return
    if (!file) {
        return false;
    }

    std::string line;
    // Read username
    if (std::getline(file, line) && !line.empty()) {
        size_t length = min(line.size(), CLIENT_NAME_SIZE - 1); // Ensure it fits in the buffer
        std::memcpy(_self.userName.user_name, line.c_str(), length);
        _self.userName.user_name[length] = '\0'; // Null-terminate exactly after username
    }
    else {
        file.close();
        return false;
    }

    // Read UUID (Hex format -> Binary)
    if (std::getline(file, line) && line.length() == (UUID_LEN * 2)) {
        if (!hexStringToBytes(line, _self.uuid.uuid, UUID_LEN)) {
            std::memset(_self.uuid.uuid, 0, UUID_LEN); // Reset to zero if invalid
            file.close();
            return false;
        }
    }
    else {
        file.close();
        return false;
    }

    // Read private key
    std::string base64key = "";
    std::string currline;
    while (std::getline(file, currline)) {
        base64key.append(currline);
    }

    _RSAWrapper = new RSAPrivateWrapper(Base64Wrapper::decode(base64key)); //decode an save
    file.close();
    return true;
}

//uses the == operator to compare two user_name structs and returns the index of the client if exists, else return -1
int Client::findClient(s_user_name user_name) {
    for (int i = 0; i < _clients.size(); i++) {
        if (user_name == _clients[i]->userName)
            return i;
    }
    return -1;
}

//uses the == operator to compare two uuid structs and returns the index of the client if exists, else return -1
int Client::findClient(s_uuid id) {
    for (int i = 0; i < _clients.size(); i++) {
        if (id == _clients[i]->uuid)
            return i;
    }
    return -1;
}

bool Client::writeInfoFile() {
    std::ofstream file(USER_INFO_FILE);
    if (!file)
        return false;

    //Write user name
    file << _self.userName.user_name << std::endl;

    // Write UUID as hexadecimal
    for (int i = 0; i < UUID_LEN; i++)
        file << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(_self.uuid.uuid[i]);
    file << std::endl;

    // Write private key
    file << Base64Wrapper::encode(_RSAWrapper->getPrivateKey()) << std::endl;

    file.close();
    return true;
}

Client::Client(const std::string& ip, const std::string& port) : _socket(_ioContext), _resolver(_ioContext) {
	boost::asio::connect(_socket, _resolver.resolve(ip, port));
    registered = readInfoFile();//Get the user info, if exists, otherwise set the members to zero
    if(registered)
        std::cout << "Connected as " << _self.userName.user_name << std::endl;
}

Client::~Client() {
    delete _RSAWrapper;
    for (int i = 0; i < _clients.size(); i++) {
        delete _clients[i];
    }
}

void printMenu() {
    std::cout << std::endl << "MessageU client at your service." << std::endl << std::endl;
    std::cout << static_cast<int>(Menu::REGISTER) << ") Register" << std::endl;
    std::cout << static_cast<int>(Menu::CLIENT_LIST) << ") Request for clients list" << std::endl;
    std::cout << static_cast<int>(Menu::PUBLIC_KEY) << ") Request for public key" << std::endl;
    std::cout << static_cast<int>(Menu::PULL_MESSAGE) << ") Request for waiting messages" << std::endl;
    std::cout << static_cast<int>(Menu::SEND_MESSAGE) << ") Send a text message" << std::endl;
    std::cout << static_cast<int>(Menu::REQ_SYM_KEY) << ") Request for symmetric key" << std::endl;
    std::cout << static_cast<int>(Menu::SEND_SYM_KEY) << ") Send your symmetric key" << std::endl;
    std::cout << static_cast<int>(Menu::EXIT) << ") Exit client" << std::endl;
}
int promptReq() {
    int choice = -1;
    while (true) {
        std::cin >> choice;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Please enter a valid number." << std::endl;
        }
        else
            return choice;
    }
    
}
void Client::run() {
    while (true) {
        printMenu();
        int req = promptReq();

        //We have to get a uuid to make requests!
        if (!registered && !(req == Menu::EXIT || req == Menu::REGISTER)) {
            std::cout << "Please register first." << std::endl;
            continue;
        }

        switch (req) {
        case Menu::REGISTER:
            if (registered)
                std::cout << "You are already registered!" << std::endl;
            else
                handleRegReq();
            break;
        case Menu::CLIENT_LIST:
            handleClientListReq();
            break;
        case Menu::PUBLIC_KEY:
            handlePublicKeyReq();
            break;
        case Menu::PULL_MESSAGE:
            handlePullMessagesReq();
            break;
        case Menu::SEND_MESSAGE:
            handleSendMessage();
            break;
        case Menu::REQ_SYM_KEY:
            handleSymKeyReq();
            break;
        case Menu::SEND_SYM_KEY:
            handleSendSymKey();
            break;
        case Menu::EXIT:
            return;
            break;
        default:
            std::cout << "Invalid request, please try again." << std::endl;
        }
    }
}

void promptUserName(char* user_name_arr) {
    std::cout << "Please enter your desired user name: " << std::endl;
    std::string strUsername; //we'll read the user name into a string for extra safety.
    std::cin.ignore();
    std::getline(std::cin, strUsername);
    while (strUsername.size() > CLIENT_NAME_SIZE - 1) {
        std::cout << "User name is too long, please enter a name shorter than 254 characters: " << std::endl;
        std::cin.ignore();
        std::getline(std::cin, strUsername);
    }
    for (int i = 0; i < strUsername.size(); i++)
        user_name_arr[i] = strUsername[i];
    user_name_arr[strUsername.size()] = '\0'; //make sure it's null terminated
}

void Client::handleRegReq() {
    promptUserName(_self.userName.user_name);
    
    //create a private key
    _RSAWrapper = new RSAPrivateWrapper();
    _RSAWrapper->getPublicKey(_self.publicKey, PUBLIC_KEY_SIZE);
    
    //create the request and send it
    std::cout << "Preparing request...";
    RequestPacket req;
    req.setHeader(_self.uuid.uuid, VERSION, static_cast<int>(Req::REGISTER), CLIENT_NAME_SIZE + PUBLIC_KEY_SIZE);
    req.appendPayload(_self.userName.user_name, CLIENT_NAME_SIZE);
    req.appendPayload(_self.publicKey, PUBLIC_KEY_SIZE);
    req.send(_socket);
    std::cout << "Request sent." << std::endl;

    //read answer
    ResponsePacket* res = ResponsePacket::receive(_socket);
    if (res->getCode() == static_cast<int>(Response::REGISTER_SUCCESS)) {
        std::copy(res->getPayload()->begin(), res->getPayload()->end(), _self.uuid.uuid);
        writeInfoFile();
        registered = true;
        std::cout << "Registerd successfully! Your user name is \"" << _self.userName.user_name << "\"." << std::endl;
    }
    else {
        std::cout << "Server responded with an error." << std::endl;
    }
    delete res;
}

void Client::handleClientListReq() {
    //Create the request and send it
    RequestPacket req;
    req.setHeader(_self.uuid.uuid, VERSION, static_cast<int>(Req::CLIENT_LIST), 0);
    req.send(_socket);

    //read answer
    ResponsePacket* res = ResponsePacket::receive(_socket);
    if (res->getCode() == static_cast<int>(Response::CLIENT_LIST)) {
        int clientsNum = (res->getPayloadSize()) / (UUID_LEN + CLIENT_NAME_SIZE);
        
        if (clientsNum == 0) {
            std::cout << "The client list is empty." << std::endl;
            return;
        }

        std::vector<uint8_t> *payload = res->getPayload();
        size_t offset = 0;
        for (int i = 0; i < clientsNum; i++) {
            // Extract UUID
            uint8_t uuidArr[UUID_LEN] = { 0 };
            std::memcpy(uuidArr, payload->data() + offset, UUID_LEN);
            offset += UUID_LEN;

            // Extract Client Name
            char clientNameArr[CLIENT_NAME_SIZE] = { 0 };
            std::memcpy(clientNameArr, payload->data() + offset, CLIENT_NAME_SIZE);
            offset += CLIENT_NAME_SIZE;

            s_uuid uuid(uuidArr);
            if (findClient(uuid) < 0) {
                //client is not saved in the client list, we'll create a new client struct for them
                s_user_name userName(clientNameArr);
                s_client *curr_client = new s_client(uuid, userName);
                _clients.push_back(curr_client);
            }
            std::cout << i + 1 << ")\t" << clientNameArr << std::endl << std::dec;
        }
    }
    else {
        std::cout << "Server responded with an error." << std::endl;
    }
    delete res;
}

s_user_name promptOtherUserName() {
    std::cout << "Please enter the user name of the user you would like to send a message to:" << std::endl;
    std::string strUsername; //we'll read the user name into a string for extra safety.
    std::cin.ignore();
    std::getline(std::cin, strUsername);
    while (strUsername.size() > CLIENT_NAME_SIZE - 1) {
        std::cout << "User name is too long, please enter a name shorter than 254 characters: " << std::endl;
        std::cin.ignore();
        std::getline(std::cin, strUsername);
    }
    char userNameArr[CLIENT_NAME_SIZE];
    for (int i = 0; i < strUsername.size(); i++)
        userNameArr[i] = strUsername[i];
    userNameArr[strUsername.size()] = '\0'; //make sure it's null terminated
    return s_user_name(userNameArr);
}

void Client::handlePublicKeyReq() {
    s_user_name otherUser = promptOtherUserName();
    int userInd = findClient(otherUser);
    if (userInd < 0) {
        std::cout << "No such user saved in the client." << std::endl;
        if (_clients.size() == 0)
            std::cout << "Please request a user list before trying to send messages." << std::endl;
        return;
    }

    //Send request
    RequestPacket req;
    req.setHeader(_self.uuid.uuid, VERSION, static_cast<int>(Req::PUBLIC_KEY), UUID_LEN);
    req.appendPayload(_clients[userInd]->uuid.uuid, UUID_LEN);
    req.send(_socket);

    //Get response
    ResponsePacket* res = ResponsePacket::receive(_socket);
    if (res->getCode() == static_cast<int>(Response::PUBLIC_KEY)) {
        std::vector<uint8_t>* payload = res->getPayload();
        // Extract UUID
        uint8_t uuidArr[UUID_LEN] = { 0 };
        std::memcpy(uuidArr, payload->data(), UUID_LEN);
        s_uuid sentUuid(uuidArr);

        if (sentUuid != _clients[userInd]->uuid) {
            std::cout << "Unexpected error: server recieved wrong id." << std::endl;
            return;
        }

        // Save private key
        std::memcpy(_clients[userInd]->publicKey, payload->data() + UUID_LEN, PUBLIC_KEY_SIZE);
        _clients[userInd]->publicKeySet = true;
        std::cout << "Public key for user \"" << _clients[userInd]->userName.user_name << "\" saved." << std::endl;
    }
    else
        std::cout << "Server responded with an error." << std::endl;
    delete res;
}

void Client::handlePullMessagesReq() {
    //Create the request and send it
    RequestPacket req;
    req.setHeader(_self.uuid.uuid, VERSION, static_cast<int>(Req::PULL_MESSAGE), 0);
    req.send(_socket);

    //read messages and handle each type
    ResponsePacket* res = ResponsePacket::receiveHeader(_socket);
    if (res->getCode() == static_cast<int>(Response::WAITING_MESSAGES)) {
        uint32_t remaining = res->getPayloadSize();
        if (remaining == 0)
            std::cout << "No new messages." << std::endl;
        while (remaining != 0) {
            if (remaining < IN_MESSAGE_HEADER_SIZE) {
                std::cout << "Error: Corrupted packet." << std::endl;
                delete res;
                return;
            }
            s_message* msg = read_message(_socket);
            int clientInd = findClient(msg->uuid);
            //Symmetric key request
            if (msg->messageType == static_cast<int>(MessageType::SYM_KEY_REQ)) {
                if (clientInd < 0)
                    std::cout << "Symmetric key request from unknown user, please update the client list by asking for it again." << std::endl;
                else {
                    std::cout << SOM_STRING << std::endl;
                    std::cout << "From: " << _clients[clientInd]->userName.user_name << std::endl;
                    std::cout << "Content:" << std::endl << "Request for symmetric key." << std::endl;
                    std::cout << EOM_STRING << std::endl << std::endl;
                    _clients[clientInd]->requestedSymKey = true;
                }
            }

            //Symmetric key sent - encrypted with RSA
            else if (msg->messageType == static_cast<int>(MessageType::SYM_KEY_SENT)) {
                if (clientInd < 0)
                    std::cout << "Symmetric key sent from unknown user, please update the client list by asking for it again." << std::endl;
                else {
                    std::cout << SOM_STRING << std::endl;
                    std::cout << "From: " << _clients[clientInd]->userName.user_name << std::endl;
                    std::cout << "Content:" << std::endl << "Symmetric key recieved." << std::endl;
                    std::string decKey = _RSAWrapper->decrypt(msg->content, msg->messageSize);
                    if (decKey.size() != SYM_KEY_SIZE)
                        std::cout << "Error: symmetric key of invalid size." << std::endl;
                    else {
                        for (int i = 0; i < SYM_KEY_SIZE; i++)
                            _clients[clientInd]->symKey[i] = decKey[i];
                        _clients[clientInd]->symKeySet = true;
                    }
                    std::cout << EOM_STRING << std::endl << std::endl;
                }
            }

            //Text message encrypted with a symmetric key
            else if (msg->messageType == static_cast<int>(MessageType::TEXT)) {
                if (clientInd < 0)
                    std::cout << "A message sent from unknown user, please " << std::endl;
                else if (!_clients[clientInd]->symKeySet) {
                    std::cout << "A meesage which can not be decrypted sent from " << _clients[clientInd]->userName.user_name << std::endl;
                    std::cout << "Try asking for a symmetric key again." << std::endl;
                }
                else {
                    AESWrapper* key = new AESWrapper(_clients[clientInd]->symKey, SYM_KEY_SIZE);
                    std::cout << SOM_STRING << std::endl;
                    std::cout << "From: " << _clients[clientInd]->userName.user_name << std::endl;
                    std::cout << "Content:" << std::endl << key->decrypt(msg->content, msg->messageSize) << std::endl;
                    std::cout << EOM_STRING << std::endl << std::endl;
                    delete key;
                }
            }
            else {
                std::cout << "Error: Unknown message type." << std::endl;
            }
            remaining = remaining - (msg->messageSize + IN_MESSAGE_HEADER_SIZE);
            delete msg;
        }
    }
    else
        std::cout << "Server responded with an error." << std::endl;
    delete res;
    
}

/*Handles sending a message to the */
void Client::handleSendMessage() {
    s_user_name otherUser = promptOtherUserName();
    int userInd = findClient(otherUser);
    if (userInd < 0) {
        std::cout << "No such user saved in the client." << std::endl;
        if (_clients.size() == 0)
            std::cout << "Please request a user list before trying to send messages." << std::endl;
        return;
    }
    if (!_clients[userInd]->symKeySet) {
        std::cout << "There is no symmetric key saved for this user, please ask for symmetric key before sending a message." << std::endl;
        return;
    }

    //get message as a string and turn it into a char array
    std::cout << "Please enter your message:" << std::endl;
    std::string strMessage;
    std::getline(std::cin, strMessage);
    uint32_t mSize = strMessage.size() + 1;
    if (mSize > 4000000) {
        //The size itself can be stored in 4 bytes, however I chose to restrict it even more because the input should be entered manually
        std::cout << "Error: Message is unreasonably long." << std::endl;
        return;
    }
    char* messageArr = new char[mSize];
    for (int i = 0; i < strMessage.size(); i++)
        messageArr[i] = strMessage[i];
    messageArr[strMessage.size()] = '\0';
    //encrypt
    AESWrapper* key = new AESWrapper(_clients[userInd]->symKey, SYM_KEY_SIZE);
    std::string encMessage = key->encrypt(messageArr, mSize);
    delete[] messageArr;
    delete key;

    //create packet and send
    RequestPacket packet = RequestPacket();
    packet.setHeader(_self.uuid.uuid, VERSION, static_cast<int>(Req::SEND_MESSAGE), OUT_MESSAGE_HEADER_SIZE + encMessage.size());
    packet.appendMessageHeader(_clients[userInd]->uuid.uuid, static_cast<int>(MessageType::TEXT), encMessage.size());
    packet.appendPayload(encMessage);
    packet.send(_socket);

    //read server response
    ResponsePacket* res = ResponsePacket::receive(_socket);
    if (res->getCode() == static_cast<int>(Response::MESSAGE_SENT))
        std::cout << "Message sent successfully!" << std::endl;
    else
        std::cout << "Server responded with an error." << std::endl;
    delete res;
}

void Client::handleSymKeyReq() {
    s_user_name otherUser = promptOtherUserName();
    int userInd = findClient(otherUser);
    if (userInd < 0) {
        std::cout << "No such user saved in the client." << std::endl;
        if (_clients.size() == 0)
            std::cout << "Please request a user list before trying to send messages." << std::endl;
        return;
    }

    //send request
    RequestPacket packet = RequestPacket();
    packet.setHeader(_self.uuid.uuid, VERSION, static_cast<int>(Req::SEND_MESSAGE), OUT_MESSAGE_HEADER_SIZE);
    packet.appendMessageHeader(_clients[userInd]->uuid.uuid, static_cast<int>(MessageType::SYM_KEY_REQ), 0);
    packet.send(_socket);

    //read server response
    ResponsePacket* res = ResponsePacket::receive(_socket);
    if (res->getCode() == static_cast<int>(Response::MESSAGE_SENT))
        std::cout << "Symmetric key request sent successfully!" << std::endl;
    else
        std::cout << "Server responded with an error." << std::endl;
    delete res;
}

void Client::handleSendSymKey() {
    s_user_name otherUser = promptOtherUserName();
    int userInd = findClient(otherUser);
    if (userInd < 0) {
        std::cout << "No such user saved in the client." << std::endl;
        if (_clients.size() == 0)
            std::cout << "Please request a user list before trying to send messages." << std::endl;
        return;
    }
    if (!_clients[userInd]->requestedSymKey) {
        std::cout << "Client hasn't requested a symmetric key, if you would like to exchange a symmetric key you have to request it first." << std::endl;
        return;
    }
    if (!_clients[userInd]->publicKeySet) {
        std::cout << "Request this user's public key first." << std::endl;
        return;
    }
    //create the key and cipher it with receiver's public key
    AESWrapper::GenerateKey(_clients[userInd]->symKey, SYM_KEY_SIZE);
    std::cout << std::endl;
    RSAPublicWrapper* pubKey = new RSAPublicWrapper(_clients[userInd]->publicKey, PUBLIC_KEY_SIZE);
    std::string encSymKey = pubKey->encrypt(reinterpret_cast<char*>(_clients[userInd]->symKey), SYM_KEY_SIZE);
    delete pubKey;

    //send the message
    RequestPacket packet = RequestPacket();
    packet.setHeader(_self.uuid.uuid, VERSION, static_cast<int>(Req::SEND_MESSAGE), OUT_MESSAGE_HEADER_SIZE + RSA_ENC_SIZE);
    packet.appendMessageHeader(_clients[userInd]->uuid.uuid, static_cast<int>(MessageType::SYM_KEY_SENT), RSA_ENC_SIZE);
    packet.appendPayload(encSymKey);
    packet.send(_socket);

    //read server response
    ResponsePacket* res = ResponsePacket::receive(_socket);
    if (res->getCode() == static_cast<int>(Response::MESSAGE_SENT)) {
        std::cout << "Symmetric key sent successfully!" << std::endl;
        _clients[userInd]->symKeySet = true;
    }
    else
        std::cout << "Server responded with an error." << std::endl;
    delete res;
}