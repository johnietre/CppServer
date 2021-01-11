#include "server.hpp"

#include <netinet/in.h>  // htons, htonl
#include <string.h>      // memset
#include <sys/socket.h>  // accept, bind, listen, send, recv, shutdown, socklen_t, sockaddr, sockaddr_in, AF_INET, SOCK_STREAM

#include <string>
using namespace std;

SocketServer::SocketServer() {}

SocketServer::SocketServer(short port) {
  ip = "localhost";
  this->port = port;
}

SocketServer::SocketServer(string ip, short port) {
  this->ip = ip;
  this->port = port;
}

bool SocketServer::isClosed() {
  return closed;
}