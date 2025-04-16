#ifndef UTILS_H
#define UTILS_H
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <boost/asio.hpp>

#define min(a,b) ((a)<(b)?(a):(b))

/*
* returns true if all characters in s are digits
*/
bool isNumeric(std::string& s);

/*
* reads the server info from filename's first line, if the line is in valid format
* returns true and saves the ip to the ip paramter and the port to the port paramater
* if fails, will return false
*/
bool readServerInfo(const std::string& filename, std::string& ip, int& port);

/*
* convert an hex string to a byte array
*/
bool hexStringToBytes(const std::string& hex, uint8_t* bytes, size_t length);

#endif