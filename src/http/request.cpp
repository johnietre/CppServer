#include "request.hpp"

using namespace std;

Request::Request() {}

char Request::getType() { return type; }

string Request::getPattern() { return pattern; }

string Request::getFullPattern() { return full_pattern; }

string Request::getFile() { return file; }