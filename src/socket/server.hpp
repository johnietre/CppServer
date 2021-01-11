#ifndef SERVER_HPP
#define SERVER_HPP

#include "conn.hpp"

#include <string>
using namespace std;

class SocketServer {
private:
  string ip;
  short port;
  bool running = false;
  bool closed;
public:
  SocketServer();
  SocketServer(short port);
  SocketServer(string ip, short port);
  bool isClosed();
};

#endif
