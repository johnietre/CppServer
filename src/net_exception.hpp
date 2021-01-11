#ifndef NET_EXCEPTION_HPP
#define NET_EXCEPTION_HPP

#include <exception>
#include <string>

struct NetException : std::exception {
private:
  std::string error;
public:
  NetException(std::string s) {error = s;}
  const char* what() const noexcept {return error.c_str();}
};

#endif
