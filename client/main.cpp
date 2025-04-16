#include "utils.h"
#include "client.h"
#include <boost/lexical_cast.hpp>
//The main method of the client program, reads the ip and port from the the server.info file and then initialises a client object with them
int main() {
    //read ip and port
    std::string filename = "server.info";
    std::string ip = "";
    int port = 0;
    if (readServerInfo(filename, ip, port)) {
        std::cout << "IP is: " << ip << std::endl;
        std::cout << "Port is: " << port << std::endl;
    }
    else {
        std::cout << "Failed to read host's details, terminating now." << std::endl;
        return 0;
    }

    //run client
    try {
        Client client(ip, std::to_string(port));//initialise client
        client.run();//run client loop
    }
    catch (std::exception& e) {
        std::cerr << "Connection error: " << e.what() << std::endl;
        return 0;
    }

    
}