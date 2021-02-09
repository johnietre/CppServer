// g++ -o main main.cpp -lpthread -std=gnu++17

#include "../../src/net_http.hpp"
#include <iostream>
#include <map>
#include <string>
#include <thread>
using namespace std;
using namespace net_http;

void indexHandler(ResponseWriter w, Request &r) {
  w.WriteFile("index.html");
}

void homeHandler(ResponseWriter w, Request &r) {
  w.WriteFile("home.html");
}

void staticHandler(ResponseWriter w, Request &r) {
  w.WriteFile("./static/" + r.getFile());
}

int main(int argc, char *argv[]) {
  HTTPServer server("localhost", 8000, 10);
  cout << server.getIPLong() << '\n';
  cout << server.getPort() << '\n';
  server.handleFunc("/", &indexHandler);
  server.handleFunc("/home", &homeHandler);
  server.handleFunc("/static", &staticHandler);
  server.setDefaultPattern("/");
  server.start();
  return 0;
}
