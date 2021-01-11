/* TODO
 * Add the different statuses (like not found, internal server error)
 * Format the status to accept content type, length, etc
 * Read the request from the socket to get the method (GET, POST) and data
 * After that, implement all that data into an easily accessable form in request
 * (ie, proper http headers) Possibly use send/recv (sys/socket.h) instead of
 * write/read to reduce size Handle for empty buffers Ex of above: in a
 * webbrowser, go to index, go to home, push back button, refresh, and push...
 * foward button
 * Look at throwing errors rather than returning error codes
 * Implement a file server
 * Implement option to either set thread count of pool or start thread per conn
 * Change from read and write (unistd.h) to recv and send (socket.h)
 * Possibly use map for content type in write file method
 */

#include "http_server.hpp"

#include <netinet/in.h> // htons, htonl
#include <string.h>     // memset
#include <sys/socket.h> // accept, bind, listen, shutdown, socklen_t, sockaddr, sockaddr_in, AF_INET, SOCK_STREAM
#include <unistd.h>     // read, close

#include <functional> // bind
#include <future>     // async
#include <iostream>   // perror

using namespace std;


const long LOCAL_HOST = 2130706433; // 127.0.0.1

HTTPServer::HTTPServer() {
  ip = htonl(LOCAL_HOST);
  port = 8080;
}

HTTPServer::HTTPServer::HTTPServer(short PORT) {
  ip = htonl(LOCAL_HOST);
  port = PORT;
}

HTTPServer::HTTPServer(string IP, short PORT) {
  setIP(IP);
  port = PORT;
}

HTTPServer::HTTPServer(long IP, short PORT) {
  ip = htonl(IP);
  port = PORT;
}

HTTPServer::HTTPServer(string IP, short PORT, int thread_count) {
  setIP(IP);
  port = PORT;
  num_threads = thread_count;
}

HTTPServer::HTTPServer(long IP, short PORT, int thread_count) {
  ip = htonl(IP);
  port = PORT;
  num_threads = thread_count;
}

HTTPServer::~HTTPServer() { stop(); }

// Starts the server
int HTTPServer::start(bool blocking) {
  if (running)
    return SERVER_ALREADY_RUNNING;
  running = true;
  // Check to make sure the default pattern has a matching function
  if (default_pattern != "") {
    if (routes[default_pattern] == nullptr) {
      cerr << "No handle function matching default pattern\n";
      exit(EXIT_FAILURE);
    }
  }
  for (int i = 0; i < num_threads; i++)
    threads.push_back(thread(
        bind(&handle_conns, &running, &sock_queue, &server_mut, &condition,
             &routes, default_pattern, allow_partial, not_found_file)));
  if (blocking)
    run_server();
  // else
  //   async(run_server);
  return 0;
}

void HTTPServer::stop() {
  if (!running)
    return;
  puts("Stopping...");
  {
    unique_lock<mutex> lock(server_mut);
    running = false;
  }
  condition.notify_all(); // wakes up all threads
  int size = threads.size();
  for (int i = 0; i < size; i++)
    threads[i].join();
  puts("Stopped");
}

// Associates a function with a slug pattern
bool HTTPServer::handleFunc(string pattern, route_handler *handler) {
  if (routes[pattern] != nullptr)
    return false;
  routes[pattern] = handler;
  return true;
}

// Sets the IP address so long as the server isn't running
// Only works with IPV4
void HTTPServer::setIP(string IP) {
  if (running)
    return;
  if (IP == "localhost") {
    ip = LOCAL_HOST;
    return;
  }
  ip = 0;
  int part_count = 1;
  string part;
  // Parse the string and calculate the integer IP
  for (const char n : IP) {
    if (n != '.')
      part += n;
    else {
      long sub = stol(part);
      for (int i = 0; i < 4 - part_count; i++)
        sub *= 256;
      ip += sub;
      part = "";
      part_count++;
    }
  }
  ip += stol(part);
  if (part_count != 4) { // Handle invalid IPV4
    cerr << "Invalid part count";
    exit(EXIT_FAILURE);
  }
  // ip = htonl(ip);
}

// Sets the IP address so long as the server isn't running
void HTTPServer::setIP(long IP) {
  if (running)
    return;
  ip = htonl(IP);
}

// Sets the port so long as the server isn't running
void HTTPServer::setPort(short PORT) {
  if (running)
    return;
  port = PORT;
}

// Sets the number of threads so long as the server isn't running
void HTTPServer::setNumThreads(int num) {
  if (running)
    return;
  num_threads = num;
}

void HTTPServer::setDefaultPattern(string pattern) {
  if (running)
    return;
  default_pattern = pattern;
}

void HTTPServer::setAllowPartial(bool allow) {
  if (running)
    return;
  allow_partial = allow;
}

void HTTPServer::setNotFoundFile(string filepath) {
  if (!running)
    not_found_file = filepath;
}

// Returns the IP address as a string
string HTTPServer::getIPString() { return ""; }

// Returns the IP address as a long integer
long HTTPServer::getIPLong() { return ip; }

// Returns the port as a short integer
short HTTPServer::getPort() { return port; }

// Returns the number of threads
int HTTPServer::getNumThreads() { return num_threads; }

string HTTPServer::getNotFoundFile() { return not_found_file; }

/* Private Methods */

// Finishes setting up and starting the server
void HTTPServer::run_server() {
  // Set up socket (some kind of way)
  struct sockaddr_in address;
  int server_fd, new_socket, addrlen = sizeof(address);
  long valread;
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    cerr << "Error in socket\n";
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(ip);
  address.sin_port = htons(port);

  memset(address.sin_zero, '\0', sizeof(address.sin_zero));

  // Bind address to the socket and start listening
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    cerr << "Error in bind\n";
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 50) < 0) {
    cerr << "Error in listen\n";
    exit(EXIT_FAILURE);
  }

  // Start the server
  puts("Starting server...");
  while (running) {
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
      cerr << "Error in accept\n";
    } else
      submit_to_pool(new_socket);
  }
}

// The function the threads will run to handle for connections
void HTTPServer::handle_conns(const bool *running_ref, queue<int> *queue_ref,
                              mutex *mutex_ref, condition_variable *cond_ref,
                              map<string, route_handler *> *routes_ref,
                              string default_ref, bool allow_ref,
                              string not_found_ref) {
  char buffer[30000] = {0}; // Buffer for reading from the socket
  int sock, valread; // the socket number and the number of bytes read (?)
  while ((*running_ref)) {
    { // Wait for the condition variable to notify the thread it can access the
      // queue to retrieve a socket
      // Must be in a seperate block for some reason
      unique_lock<mutex> lock(*mutex_ref);
      cond_ref->wait(lock, [queue_ref, running_ref] {
        return !queue_ref->empty() || !(*running_ref);
      });
      if (!(*running_ref))
        break;
      sock = queue_ref->front();
      queue_ref->pop();
    }
    // Parse the request and send it to the appropriate handler
    valread = read(sock, buffer, 30000);
    Request req = parse_header(buffer);
    route_handler *handler;
    if (req.pattern == "/")
      handler = (*routes_ref)["/"];
    else {
      string prev, curr;
      for (const string slug : req.slugs) {
        curr += '/' + slug;
        handler = (*routes_ref)[curr];
        if (handler == nullptr) {
          if (allow_ref)
            handler = (*routes_ref)[prev];
          break;
        }
        prev = curr;
      }
    }
    if (handler == nullptr) {
      if (default_ref != "")
        (*routes_ref)[default_ref](ResponseWriter(sock), req);
      else
        ResponseWriter(sock).PageNotFound(not_found_ref);
    } else
      handler(ResponseWriter(sock), req);
    shutdown(sock, SHUT_WR);
    close(sock);
  }
}

// Parses the request header and returns a Request object
Request HTTPServer::parse_header(string header) {
  string part;
  Request r;
  for (const char c : header) {
    if (c == ' ') {
      if (part == "GET")
        r.type = 'G';
      else if (part == "POST")
        r.type = 'P';
      else if (part == "HTTP")
        return r;
      else if (part[0] == '/') {
        if (part == "/") {
          r.pattern = "/";
          r.slugs.push_back("/");
        } else {
          string slug = "";
          for (const char p : part.substr(1)) {
            if (p == '/') {
              r.slugs.push_back(slug);
              r.pattern += '/' + slug;
              slug = "";
            } else
              slug += p;
          }
          if (slug.rfind(".") != string::npos)
            r.file = slug;
          else {
            r.slugs.push_back(slug);
            r.pattern += '/' + slug;
          }
        }
        r.full_pattern = part;
        part = "";
      }
      part = "";
    } else
      part += c;
  }
  return r;
}

// Sends a socket to the thread pool
void HTTPServer::submit_to_pool(int sock) {
  { // Locks the mutex in order to send a socket to the queue and notify one of
    // the threads
    unique_lock<mutex> lock(server_mut);
    sock_queue.push(sock);
  }
  condition.notify_one();
}
