#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "http_server.hpp"

#include <string>
#include <vector>
using namespace std;

class Request {
private:
  char type = 'G';
  string pattern = "";
  string full_pattern = "";
  string file = "";
  vector<string> slugs;

  friend class HTTPServer;

public:
  Request();
  Request(char req_type, string req_pattern);
  char getType();
  string getPattern();
  string getFullPattern();
  string getFile();
};

#endif
