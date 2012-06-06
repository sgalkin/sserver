#ifndef SSERVER_FD_H_INCLUDED
#define SSERVER_FD_H_INCLUDED

#include "exception.h"
#include <boost/utility.hpp>
#include <boost/optional.hpp>
#include <unistd.h>
#include <fcntl.h>

class FD : boost::noncopyable {
    friend void swap(FD&, FD&);

public:
    FD() : fd_(-1) {}
    explicit FD(int fd, bool blocking = true) : fd_(fd) {
        REQUIRE(fd_ != -1, "Invalid file descriptor");
        if(blocking) set_flag(O_NONBLOCK);
    }

    ~FD() { close(fd_); }
    
    int get() const { return fd_; }
    operator int() const { return get(); }
    void reset() { FD().swap(*this); }
    void reset(int fd, bool blocking = true) { FD(fd, blocking).swap(*this); }
    void swap(FD& fd) { std::swap(fd_, fd.fd_); }

    int release() {
        int fd = fd_;
        fd_ = -1;
        return fd;
    }

private:
    void set_flag(long flag) {
        int flags = 0;
        CHECK_CALL((flags = fcntl(fd_, F_GETFL, 0)), "fcntl/GETFL(" << fd_ << ")");
        CHECK_CALL(fcntl(fd_, F_SETFL, (flags | flag)), "fcntl/SETFL(" << fd_ << ")");
    }

    int fd_;
};

inline void swap(FD& lhs, FD& rhs) {
    lhs.swap(rhs);
}

class Pipe {
public:
    Pipe() {
        int fds[2];
        CHECK_CALL(pipe(fds), "pipe");
        read_.reset(fds[0]);
        write_.reset(fds[1]);
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
