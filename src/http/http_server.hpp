#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include "request.hpp"
#include "response_writer.hpp"

#include <condition_variable> // condition_variable
#include <map>                // map
#include <mutex>              // mutex, unique_lock
#include <queue>              // queue
#include <string>             // string, stol
#include <thread>             // thread
#include <vector>             // vector

using namespace std;

enum {
  ENUM_1,
  SERVER_FILE_NOT_FOUND,
  SERVER_INVALID_FILE,
};

enum {
  ENUM_2,
  SERVER_ALREADY_RUNNING,
};

typedef void route_handler(ResponseWriter w, Request &r);

class HTTPServer {
private:
  long ip;
  short port;
  int num_threads = thread::hardware_concurrency();
  bool running = false;
  string default_pattern = "";
  bool allow_partial = false;
  string not_found_file = "";
  mutex server_mut;
  condition_variable condition;
  vector<thread> threads;
  queue<int> sock_queue;
  map<string, route_handler *> routes;

  void run_server();
  static void handle_conns(const bool *done_ref, queue<int> *queue_ref,
                           mutex *mutex_ref, condition_variable *cond_ref,
                           map<string, route_handler *> *routes_ref,
                           string default_ref, bool allow_ref,
                           string not_found_ref);
  static Request parse_header(string header);
  static void page_not_found(int sock);
  void submit_to_pool(int sock);

public:
  HTTPServer();
  HTTPServer(short PORT);
  HTTPServer(string IP, short PORT);
  HTTPServer(long IP, short PORT);
  HTTPServer(string IP, short PORT, int thread_count);
  HTTPServer(long IP, short PORT, int thread_count);
  ~HTTPServer();
  int start(bool blocking = true);
  void stop();
  bool handleFunc(string pattern, route_handler *handler);
  void setIP(string IP);
  void setIP(long IP);
  void setPort(short PORT);
  void setNumThreads(int num);
  void setDefaultPattern(string pattern);
  void setAllowPartial(bool allow);
  void setNotFoundFile(string filepath);
  string getIPString();
  long getIPLong();
  short getPort();
  int getNumThreads();
  string getNotFoundFile();
};

#endif