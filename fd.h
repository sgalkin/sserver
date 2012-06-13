#ifndef SSERVER_FD_H_INCLUDED
#define SSERVER_FD_H_INCLUDED

#include "exception.h"
#include <unistd.h>
#include <fcntl.h>

// C++11 move semantics is better, but not available by default
template<typename Base>
struct Ref {
    Ref(int fd) : fd(fd) {}
    int fd;
};

class FD;

typedef Ref<FD> FDRef;

class FD {
public:
    FD() : fd_(-1) {}
    FD(FDRef fd) : fd_(fd.fd) {}
    FD(FD& fd) : fd_(fd.release()) {} // should be FD&&

    explicit FD(int fd, bool blocking = true) : fd_(fd) {
        REQUIRE(fd_ != -1, "Invalid file descriptor");
        if(blocking) set_flag(O_NONBLOCK);
    }
    ~FD() {
        if(fd_ != -1) close(fd_);
    }

    FD& operator=(FD& fd) {
        FD(fd).swap(*this);
        return *this;
    }

    operator FDRef() {
        return FDRef(release());
    }

    int get() const {
        return fd_;
    }

//    operator int() const { return get(); }

    void reset() {
        FD().swap(*this);
    }

    void reset(int fd, bool blocking = true) {
        FD(fd, blocking).swap(*this);
    }

    void swap(FD& fd) {
        std::swap(fd_, fd.fd_);
    }

    int release() {
        int fd = fd_;
        fd_ = -1;
        return fd;
    }

    int write(const char* data, size_t size) {
        return io(&::write, data, size);
    }

    int read(char* buf, size_t size) {
        return io(&::read, buf, size);
    }

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

private:
    void set_flag(long flag) {
        int flags = 0;
        CHECK_CALL((flags = fcntl(fd_, F_GETFL, 0)), "fcntl/GETFL(" << fd_ << ")");
        CHECK_CALL(fcntl(fd_, F_SETFL, (flags | flag)), "fcntl/SETFL(" << fd_ << ")");
    }

    int fd_;
};

inline std::ostream& operator<<(std::ostream& o, const FD& fd) {
    return o << fd.get();
}

// inline void swap(FD& lhs, FD& rhs) {
//     lhs.swap(rhs);
// }

class Pipe {
public:
    Pipe() {
        int fds[2];
        CHECK_CALL(pipe(fds), "pipe");
        reader_.reset(fds[0]);
        writer_.reset(fds[1]);
    }

    int write(const char* data, size_t size) {
        return writer_.write(data, size);
    }

    int read(char* buf, size_t size) {
        return reader_.read(buf, size);
    }

    const FD& writer() const { return writer_; }
    const FD& reader() const { return reader_; }
    FD& writer() { return writer_; }
    FD& reader() { return reader_; }

private:
    FD reader_;
    FD writer_;
};

#endif //SSERVER_FD_H_INCLUDED
