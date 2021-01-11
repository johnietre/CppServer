#include "response_writer.hpp"

#include <netinet/in.h> // htons, htonl
#include <string.h>     // memset
#include <sys/socket.h> // accept, bind, listen, shutdown, socklen_t, sockaddr, sockaddr_in, AF_INET, SOCK_STREAM
#include <unistd.h>     // read, close

#include <istream>
#include <filesystem>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;

const int bufferLen = 5000;
const string STATUS_OK = "HTTP/1.1 200 OK\r\n";
const string STATUS_NOT_FOUND = "HTTP/1.1 404 Not Found\r\n";
const string HTML_404 = "<!DOCTYPE HTML>\n"
                        "<html>"
                        "<head><title>404 Not Found</title></head>"
                        "<body>"
                        "<h1>404 Not Found</h1>"
                        "<p>The requested URL was not found on this server.</p>"
                        "</body></html>";
const string days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const string months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

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
  ifstream file(filepath, ifstream::binary);
  if (!file.is_open())
    return SERVER_FILE_NOT_FOUND;
  string response = STATUS_OK;
  // Get the time as a string
  response += get_time_string();
  // Check what type the file is
  size_t pos = filepath.rfind(".");
  if (pos == string::npos)
    return SERVER_INVALID_FILE;
  string ext = filepath.substr(pos);
  if (ext == ".html" || ext == ".htm")
    response += "CONTENT-TYPE: text/html; charset=utf-8\r\n";
  else if (ext == ".css")
    response += "CONTENT-TYPE: text/css; charset=utf-8\r\n";
  else if (ext == ".js")
    response += "CONTENT-TYPE: text/javascript; charset=utf-8\r\n";
  else if (ext == ".ico")
    response += "CONTENT-TYPE: image/vnd.microsoft.icon\r\n";
  else if (ext == ".png")
    response += "CONTENT-TYPE: image/png\r\n";
  else if (ext == ".jpg" || ext == ".jpeg")
    response += "CONTENT-TYPE: image/jpeg\r\n";
  else if (ext == ".json")
    response += "CONTENT-TYPE: application/json; charset=utf-8\r\n";
  else if (ext == ".csv")
    response += "CONTENT-TYPE: text/csv; charset=utf-8\r\n";
  else if (ext == ".pdf")
    response += "CONTENT-TYPE: application/pdf\r\n";
  else
    response += "CONTENT-TYPE: text/plain; charset=utf-8\r\n";
  // Find out the file size; POSSIBLY CALCULATE SIZE AS FILE IS READ IN
  unsigned long len = fs::file_size(filepath);
  response += "CONTENT-LENGTH: " + to_string(len) + "\r\n";
  response += "\r\n";
  // Send the response to the socket
  write(sock, response.c_str(), response.length());
  // Read the contents of the file and append them to the response
  char *buffer = new char[bufferLen];
  for (; len > bufferLen; len -= bufferLen) {
    file.read(buffer, bufferLen);
    write(sock, buffer, bufferLen);
  }
  if (len) {
    file.read(buffer, len);
    write(sock, buffer, len);
  }
  delete[] buffer;
  file.close();
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
