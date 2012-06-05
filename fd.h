#ifndef SSERVER_FD_H_INCLUDED
#define SSERVER_FD_H_INCLUDED

#include "exception.h"
#include <boost/utility.hpp>
#include <boost/optional.hpp>
#include <unistd.h>
#include <fcntl.h>


class FD : boost::noncopyable {
public:
  explicit FD(int fd = -1) : fd_(fd) {}

  ~FD() {
    close(fd_);
  }
    
  int get() const {
    return fd_;
  }
  
  void reset(int fd = -1) { 
    close(fd_);
    fd_ = fd; 
  }

  int release() {
    int fd = fd_;
    fd_ = -1;
    return fd;
  }

  operator int() const {
    return get();
  }

  void set_nonblock() {
    int flags = 0;
    CHECK_CALL((flags = fcntl(fd_, F_GETFL, 0)), "fcntl/GETFL(" << fd_ << ")");
    CHECK_CALL(fcntl(fd_, F_SETFL, flags | O_NONBLOCK), "fcntl/SETFL(" << fd_ << ")");
  }

private:
  int fd_;
};

class Pipe {
public:
  Pipe() {
    int fds[2];
    CHECK_CALL(pipe(fds), "pipe");
    read_.reset(fds[0]);
    write_.reset(fds[1]);
    read_.set_nonblock();
    write_.set_nonblock();
  }

  // bool write(const std::string& /*data*/) {
  //   return false;
  // }

  // boost::optional<std::string> read() {
  //   return boost::none;
  // }

private:
  FD read_;
  FD write_;
};

#endif //SSERVER_FD_H_INCLUDED
