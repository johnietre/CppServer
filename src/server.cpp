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
 */

#include "server.hpp"

#include <netinet/in.h>  // htons, htonl
#include <string.h>      // memset
#include <sys/socket.h>  // accept, bind, listen, shutdown, socklen_t, sockaddr, sockaddr_in, AF_INET, SOCK_STREAM
#include <unistd.h>      // read, close

#include <ctime>       // time_t, struct tm
#include <filesystem>  // file_size
#include <fstream>     // ifstream
#include <functional>  // bind
#include <future>      // async
#include <iostream>    // perror

using namespace std;
using namespace Server;
namespace fs = std::filesystem;

const long LOCAL_HOST = 2130706433;  // 127.0.0.1
const string STATUS_OK = "HTTP/1.1 200 OK\r\n";
const string STATUS_NOT_FOUND = "HTTP/1.1 404 Not Found\r\n";
const string days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const string months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const string HTML_404 =
    "<!DOCTYPE HTML>\n"
    "<html>"
    "<head><title>404 Not Found</title></head>"
    "<body>"
    "<h1>404 Not Found</h1>"
    "<p>The requested URL was not found on this server.</p>"
    "</body></html>";

///////////////////////////// Request /////////////////////////////

Request::Request() {}

char Request::getType() { return type; }

string Request::getPattern() { return pattern; }

string Request::getFullPattern() { return full_pattern; }

string Request::getFile() { return file; }

///////////////////////////// ResponseWriter /////////////////////////////

ResponseWriter::ResponseWriter(int sock_no) { sock = sock_no; }

// Writes plain text to the socket
int ResponseWriter::WriteText(string text) {
  string response = STATUS_OK;
  // Get the time as a string
  response += get_time_string();
  response += "CONTENT-TYPE: text/plain; charset=utf-8\r\n";
  response += "CONTENT-LENGTH: " + to_string(text.length()) + "\r\n";
  response += "\r\n";
  response += text;
  write(sock, response.c_str(), response.length());
  return 0;
}

// Finds the file with the filepath, reads it in, and writes it to the socket
int ResponseWriter::WriteFile(string filepath) {
  ifstream file(filepath);
  if (!file.is_open()) return SERVER_FILE_NOT_FOUND;
  string response = STATUS_OK;
  // Get the time as a string
  response += get_time_string();
  // Check what type the file is
  size_t pos = filepath.rfind(".");
  if (pos == string::npos) return SERVER_INVALID_FILE;
  string ext = filepath.substr(pos);
  if (ext == ".html" || ext == ".htm")
    response += "CONTENT-TYPE: text/html; charset=utf-8\r\n";
  else if (ext == ".css")
    response += "CONTENT-TYPE: text/css; charset=utf-8\r\n";
  else if (ext == ".js")
    response += "CONTENT-TYPE: text/javascript; charset=utf-8\r\n";
  else
    response += "CONTENT-TYPE: text/plain; charset=utf-8\r\n";
  // Find out the file size; POSSIBLY CALCULATE SIZE AS FILE IS READ IN
  unsigned long len = fs::file_size(filepath);
  response += "CONTENT-LENGTH: " + to_string(len) + "\r\n";
  response += "\r\n";
  // Read the contents of the file and append them to the response
  char *buffer = new char[len];
  file.read(buffer, len);
  response += buffer;
  delete[] buffer;
  file.close();
  // Send the response to the socket
  write(sock, response.c_str(), response.length());
  return 0;
}

int ResponseWriter::PageNotFound(string filepath) {
  string response = STATUS_NOT_FOUND;
  response += get_time_string();
  response += "CONTENT-TYPE: text/html; charset=utf-8\r\n";
  if (filepath == "") { // Send the default html
    response += "CONTENT-LENGTH:  " + to_string(HTML_404.length()) + "\r\n";
    response += "\r\n";
    response += HTML_404;
  } else {
    ifstream file(filepath);
    if (!file.is_open()) { // If the file isn't found, go to the default
      PageNotFound("");
      return SERVER_FILE_NOT_FOUND;
    }
    // Read the page not found file
    unsigned long len = fs::file_size(filepath);
    char *buffer = new char[len];
    file.read(buffer, len);
    response += buffer;
    delete[] buffer;
    file.close();
  }
  // Send response to the socket
  write(sock, response.c_str(), response.length());
  return 0;
}

/* Private Methods */

// Returns an HTTP header compliant string representation of the current date
// Possibly integrate into write functions
string ResponseWriter::get_time_string() {
  // Possibly use C-String since string will always 38B
  time_t t = time(nullptr);
  tm *gmtm = gmtime(&t);
  string stime = "Date: ";
  stime += days[gmtm->tm_wday] + ", ";
  stime += (gmtm->tm_mday > 9) ? to_string(gmtm->tm_mday)
                               : '0' + to_string(gmtm->tm_mday);
  stime += ' ' + months[gmtm->tm_mon] + ' ';
  stime += to_string(gmtm->tm_year + 1900) + ' ';
  stime += (gmtm->tm_hour > 9) ? to_string(gmtm->tm_hour) + ':'
                               : '0' + to_string(gmtm->tm_hour) + ':';
  stime += (gmtm->tm_min > 9) ? to_string(gmtm->tm_min) + ':'
                              : '0' + to_string(gmtm->tm_min) + ':';
  stime += (gmtm->tm_sec > 9) ? to_string(gmtm->tm_sec) + ':'
                              : '0' + to_string(gmtm->tm_sec) + ':';
  stime += " GMT\r\n";
  return stime;
}

///////////////////////////// HTTPServer /////////////////////////////

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

HTTPServer::~HTTPServer() {
  if (!running) return;
  {
    unique_lock<mutex> lock(server_mut);
    running = false;
  }
  condition.notify_all();  // wakes up all threads
  int size = threads.size();
  for (int i = 0; i < size; i++) threads[i].join();
}

// Starts the server
int HTTPServer::start(bool blocking) {
  if (running) return SERVER_ALREADY_RUNNING;
  running = true;
  // Check to make sure the default pattern has a matching function
  if (default_pattern != "") {
    if (routes[default_pattern] == nullptr) {
      cerr << "No handle function matching default pattern\n";
      exit(EXIT_FAILURE);
    }
  }
  for (int i = 0; i < num_threads; i++)
    threads.push_back(
        thread(bind(&handle_conns, &running, &sock_queue, &server_mut,
                    &condition, &routes, default_pattern, allow_partial, not_found_file)));
  if (blocking) run_server();
  // else
  //   async(test);
  return 0;
}

// Associates a function with a slug pattern
bool HTTPServer::handleFunc(string pattern, route_handler *handler) {
  if (routes[pattern] != nullptr) return false;
  routes[pattern] = handler;
  return true;
}

// Sets the IP address so long as the server isn't running
// Only works with IPV4
void HTTPServer::setIP(string IP) {
  if (running) return;
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
      for (int i = 0; i < 4 - part_count; i++) sub *= 256;
      ip += sub;
      part = "";
      part_count++;
    }
  }
  ip += stol(part);
  if (part_count != 4) {  // Handle invalid IPV4
    cerr << "Invalid part count";
    exit(EXIT_FAILURE);
  }
  // ip = htonl(ip);
}

// Sets the IP address so long as the server isn't running
void HTTPServer::setIP(long IP) {
  if (running) return;
  ip = htonl(IP);
}

// Sets the port so long as the server isn't running
void HTTPServer::setPort(short PORT) {
  if (running) return;
  port = PORT;
}

// Sets the number of threads so long as the server isn't running
void HTTPServer::setNumThreads(int num) {
  if (running) return;
  num_threads = num;
}

void HTTPServer::setDefaultPattern(string pattern) {
  if (running) return;
  default_pattern = pattern;
}

void HTTPServer::setAllowPartial(bool allow) {
  if (running) return;
  allow_partial = allow;
}

void HTTPServer::setNotFoundFile(string filepath) {
  if (!running) not_found_file = filepath;
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
                              string default_ref, bool allow_ref, string not_found_ref) {
  char buffer[30000] = {0};  // Buffer for reading from the socket
  int sock, valread;  // the socket number and the number of bytes read (?)
  while ((*running_ref)) {
    {  // Wait for the condition variable to notify the thread it can access the
      // queue to retrieve a socket
      // Must be in a seperate block for some reason
      unique_lock<mutex> lock(*mutex_ref);
      cond_ref->wait(lock, [queue_ref, running_ref] {
        return !queue_ref->empty() || !(*running_ref);
      });
      if (!(*running_ref)) break;
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
          if (allow_ref) handler = (*routes_ref)[prev];
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

// Sends the HTML_404 contents to the socket
// SUBJECT TO CHANGE OR BE REMOVED
void HTTPServer::page_not_found(int sock) {
  write(sock, (STATUS_NOT_FOUND + HTML_404).c_str(),
        (STATUS_NOT_FOUND + HTML_404).length());
}

// Sends a socket to the thread pool
void HTTPServer::submit_to_pool(int sock) {
  {  // Locks the mutex in order to send a socket to the queue and notify one of
    // the threads
    unique_lock<mutex> lock(server_mut);
    sock_queue.push(sock);
  }
  condition.notify_one();
}
