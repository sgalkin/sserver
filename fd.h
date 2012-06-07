#ifndef SSERVER_FD_H_INCLUDED
#define SSERVER_FD_H_INCLUDED

#include "exception.h"
//TODO: now it's overkill
// #include "traits.h"
#include <boost/utility.hpp>
#include <boost/optional.hpp>
#include <unistd.h>
#include <fcntl.h>

class FD : boost::noncopyable {
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

    // int result = 0;
    // for(int done = 0; done < size; done += result) {
    //     CHECK_NONBLOCK_CALL((result = ::write(fd_, data + done, size - done)), "write");
    //     if(result == -1) return done;
    // }
    int write(const char* data, size_t size) {
        return io(&::write, data, size);
    }

    int read(char* buf, size_t size) {
        return io(&::read, buf, size);
    }

    // template<typename T>
    // size_t write(const std::string& data,
    //              size_t offset = 0,
    //              typename IsByteArray<T>::type* = 0) {
    //     return offset;
    // }

private:
    template<typename Op, typename Data>
    int io(Op operation, Data data, size_t size) {
        int result = 0;
        for(size_t done = 0; done < size; done += result) {
            result = operation(fd_, data + done, size - done);
            if(result == 0 || (result == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)))
                return done;
            CHECK_CALL(result, "io(" << fd_ << ")");
        }
        return size;        
    }

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

    int write(const char* data, size_t size) {
        return write_.write(data, size);
    }

    int read(char* buf, size_t size) {
        return read_.read(buf, size);
    }

    const FD& writer() { return write_; }
    const FD& reader() { return read_; }

private:
    FD read_;
    FD write_;
};

#endif //SSERVER_FD_H_INCLUDED
