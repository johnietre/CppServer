#ifndef CONN_HPP
#define CONN_HPP

#include <string>
using namespace std;

Conn Dial(string addr);

class Conn {
private:
  int sock;
public:
  Conn();
  bool Write();
};

#endif
