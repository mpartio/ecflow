#ifndef HTTP_SERVER_EXCEPTION
#define HTTP_SERVER_EXCEPTION

#include <exception>
#include <string>

class HttpServerException : public std::exception {
  public:
   HttpServerException() = delete;
   HttpServerException(int code, const std::string& msg) : code_(code), msg_(msg) {
   }

   const char* what() const noexcept override {
	  return msg_.c_str();
   }
   int code() const {
	  return code_;
   }

  private:
   int code_;
   std::string msg_;
};

#endif
