#ifndef CONN_HPP
#define CONN_HPP

#include "../net_exception.hpp"

#include <string>
using namespace std;

class Conn {
private:
  int sock;
  bool closed = true;
  size_t maxBufferSize = 5000;
public:
  Conn();
  ~Conn();
  void close();
  void dial();
  ssize_t read(char *buffer, size_t size, bool blocking=true);
  // string readAll(bool blocking=true);
  ssize_t write(const char *buffer, size_t size=0);
  ssize_t write(const string &msg);
};

#endif
