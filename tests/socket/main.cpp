#include "../../src/net_socket.hpp"
#include <cstring>
#include <iostream>
#include <string>
using namespace std;

int main(int argc, char **argv) {
  // using namespace net_socket;
  namespace ns = net_socket;
  ns::Listener ln("localhost", 8000);
  ns::Conn conn = ln.acceptConn();
  char buff[1024];
  conn.read(buff, 1024, false);
  cout << buff << endl;
  char b[] = "goodbye";
  conn.write(b, strlen(b));
  return 0;
}
