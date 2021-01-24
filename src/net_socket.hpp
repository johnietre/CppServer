#ifndef NET_SOCKET_HPP
#define NET_SOCKET_HPP

#include "net_exception.hpp"

#include <arpa/inet.h> // inet_pton, inet_ntop
#include <netinet/in.h>  // htons, htonl
#include <sys/socket.h>  // accept, bind, listen, send, recv, shutdown, socklen_t, sockaddr, sockaddr_in, AF_INET, SOCK_STREAM

#include <cerrno> // errno
#include <cstring>      // memset, strlen
#include <string>
using namespace std;

namespace net_socket {

class Conn {
private:
  int sock;
  string ip;
  short port;
  bool closed;
  size_t maxBufferSize = 5000;
  Conn();
  void dial();
  friend class Listener;
public:
  Conn(string ip);
  Conn(short port);
  Conn(string ip, short port);
  ~Conn();
  void close();
  string getIP();
  short getPort();
  bool isClosed();
  ssize_t read(char *buffer, size_t size, bool blocking=true);
  // string readAll(bool blocking=true);
  ssize_t write(const char *buffer, size_t size=0);
  ssize_t write(const string &msg);
};

Conn::Conn(string ip) {
  if (ip == "localhost")
    this->ip = "127.0.0.1";
  else
    this->ip = ip;
  port = 8000;
  try {
    this->dial();
  } catch (exception &e) {
    throw NetException(e.what());
  }
}

Conn::Conn(short port) {
  if (port < 0)
    throw NetException("Invalid port");
  this->port = port;
  ip = "127.0.0.1";
  try {
    this->dial();
  } catch (exception &e) {
    throw NetException(e.what());
  }
}

Conn::Conn(string ip, short port) {
  if (port < 0)
    throw NetException("Invalid port");
  if (ip == "localhost")
    this->ip = "127.0.0.1";
  else
    this->ip = ip;
  this->port = port;
  try {
    this->dial();
  } catch (exception &e) {
    throw NetException(e.what());
  }
}

Conn::~Conn() {
  close();
}

void Conn::close() {
  if (!closed)
    shutdown(sock, SHUT_RDWR);
  closed = true;
}

string Conn::getIP() {
  return ip;
}

short Conn::getPort() {
  return port;
}

bool Conn::isClosed() {
  return closed;
}

ssize_t Conn::read(char *buffer, size_t size, bool blocking) {
  if (closed)
    throw NetException("connection closed");
  ssize_t r = recv(sock, buffer, size, (blocking) ? 0 : MSG_DONTWAIT);
  if (r == -1) {
    int e = errno;
    if (e != EAGAIN && e != EWOULDBLOCK) {
      closed = (e == ECONNREFUSED);
      throw NetException(strerror(e));
    }
  }
  return r;
}

/*
string Conn::readAll(bool blocking) {
  if (closed)
    throw "connection closed";
  char *buffer = new char[maxBufferSize];
  string str = buffer;
  delete[] buffer;
  return str;
}
*/

ssize_t Conn::write(const char *buffer, size_t size) {
  if (closed)
    throw "connection closed";
  size_t l = strlen(buffer);
  ssize_t sent;
  if (size < 1) {
    for (; l > maxBufferSize; l -= maxBufferSize)
      sent = send(sock, buffer, maxBufferSize, 0);
    if (l != 0)
      sent = send(sock, buffer, l, 0);
  } else {
    sent = send(sock, buffer, size, 0);
  }
  if (sent == -1) {
    int e = errno;
    closed = (e == ECONNREFUSED);
    throw NetException(strerror(e));
  }
  return sent;
}

ssize_t Conn::write(const string &str) {
  return write(str.c_str());
}

/* Private Methods */

Conn::Conn() {
  closed = false;
}

// Possibly throw errors from errno and strerr
void Conn::dial() {
  struct sockaddr_in addr;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    throw NetException("Socket creation error");
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  // Convert IP to binary
  if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0)
    throw NetException("Invalid IP address");
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    throw NetException("Error connecting");
  closed = false;
}

class Listener {
private:
  string ip;
  short port;
  int sock;
  int server_fd;
  bool closed;
  void serve();
public:
  Listener();
  Listener(string ip);
  Listener(short port);
  Listener(string ip, short port);
  ~Listener();
  void close();
  bool isClosed();
  Conn acceptConn();
};

Listener::Listener() {}

Listener::Listener(string ip) {
  if (ip == "localhost")
    this->ip = "127.0.0.1";
  else
    this->ip = ip;
  serve();    
}

Listener::Listener(short port) {
  ip = "127.0.0.1";
  this->port = port;
}

Listener::Listener(string ip, short port) {
  if (ip == "localhost")
    this->ip = "127.0.0.1";
  else
    this->ip = ip;
  this->port = port;
  serve();  
}

Listener::~Listener() {
  if (!closed)
    shutdown(sock, SHUT_RDWR);
  closed = true;
}

void Listener::close() {
  if (!closed)
    shutdown(sock, SHUT_RDWR);
  closed = true;
}

bool Listener::isClosed() {
  return closed;
}

Conn Listener::acceptConn() {
  if (closed)
    throw NetException("Socket closed");
  struct sockaddr_in addr;
  int new_socket, addrlen = sizeof(addr);
  if ((new_socket = accept(server_fd, (struct sockaddr *)&addr, (socklen_t *)&addrlen)) < 0)
    throw NetException("Error in accept");
  Conn c;
  char buff[64];
  // Possibly handle NULL return value (indicates error)
  inet_ntop(AF_INET, &addr, buff, 64);
  c.ip = buff;
  c.sock = new_socket;
  c.port = ntohs(addr.sin_port);
  return c;
}

void Listener::serve() {
  struct sockaddr_in addr;
  int server_fd, new_socket, addrlen = sizeof(addr);
  long valread;
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    throw NetException("Socket failed");
  addr.sin_family = AF_INET;
  if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0)
    throw NetException("Invalid IP address");
  addr.sin_port = htons(port);

  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  // Bind address to the socket and start listening
  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    throw NetException("Error binding to address");
  if (listen(server_fd, 50) < 0)
    throw NetException("Error listening");
  this->server_fd = server_fd;
  closed = false;
}

};

#endif
