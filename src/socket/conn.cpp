#include "conn.hpp"

#include <netinet/in.h>  // htons, htonl
#include <sys/socket.h>  // accept, bind, listen, recv, send, shutdown, socklen_t, sockaddr, sockaddr_in, AF_INET, SOCK_STREAM

#include <cerrno> // errno
#include <cstring>      // memset, strlen
#include <exception>

using namespace std;

Conn::Conn() {}

Conn::~Conn() {
  close();
}

void Conn::close() {
  if (!closed)
    shutdown(sock, SHUT_RDWR);
  closed = true;
}

void Conn::dial() {
  if (!closed)
    shutdown(sock, SHUT_RDWR);
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