#ifndef RESPONSE_WRITER_HPP
#define RESPONSE_WRITER_HPP

#include "http_server.hpp"

#include <string>
using namespace std;

class ResponseWriter {
private:
  int sock;

  static string get_time_string();

public:
  ResponseWriter(int sock_no);
  int WriteText(string text);
  int WriteFile(string filepath);
  int PageNotFound(string filepath = "");
};

#endif
