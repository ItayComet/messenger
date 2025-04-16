#include "utils.h"

bool isNumeric(std::string& s) {
	return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

bool readServerInfo(const std::string& filename, std::string& ip, int& port) {
	//open file
	std::ifstream file(filename);
	if (!file) {
		std::cerr << "Error: Could not open " << filename << std::endl;
		return false;
	}

	//get the IP:port string
	std::string line;
	std::getline(file, line);	//getline with a string is overflow safe :)
	file.close();

	if (line.empty()) {
		std::cerr << "Error: File is empty or unreadable" << std::endl;
		return false;
	}

	//read the line as a stream
	std::istringstream ss(line);
	std::string portStr;

	if (!(std::getline(ss, ip, ':') && std::getline(ss, portStr))) {
		std::cerr << "Error: Invalid IP format (expected IP:port)" << std::endl;
		return false;
	}

	if (!isNumeric(portStr)) {
		std::cerr << "Error: Port is not a valid number" << std::endl;
		return false;
	}

	//convert to int
	port = std::stoi(portStr);

	//Validate IP address using Boost lib
	boost::system::error_code ec;
	boost::asio::ip::make_address(ip, ec);
	if (ec) {
		std::cerr << "Error : Invalid IP address format" << std::endl;
		return false;
	}

	//validate port
	if (port < 1 || port > 65535) {
		std::cerr << "Error: port is out of range." << std::endl;
		return false;
	}
	return true;
}

bool hexStringToBytes(const std::string& hex, uint8_t* bytes, size_t length) {
	if (hex.length() != length * 2) {
		return false;  // Incorrect size
	}
	for (size_t i = 0; i < length; ++i) {
		std::istringstream iss(hex.substr(i * 2, 2));
		int byteValue;
		if (!(iss >> std::hex >> byteValue)) {
			return false;  // Invalid hex character
		}
		bytes[i] = static_cast<uint8_t>(byteValue);
	}
	return true;
}