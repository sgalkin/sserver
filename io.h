#ifndef SSERVER_IO_H_INCLUDED
#define SSERVER_IO_H_INCLUDED

#include "log.h"
#include "socket.h"
#include <boost/function.hpp>
#include <string>
#include <memory>
#include <utility>
#include <poll.h>

template<int io_event>
struct IOEvent { enum { Event = io_event }; };

typedef IOEvent<POLLIN> Input;
typedef IOEvent<POLLOUT> Output;


template<typename FDType>
inline std::string get_error(FDType* fd) {
    std::stringstream s;
    s << "Unexpected error";
    return s.str();
}

inline std::string get_error(TCPSocket* fd) { return fd->error(); }
inline std::string get_error(UDPSocket* fd) { return fd->error(); }


template<typename FDType>
class Control {
public:
    Control() : eof_(false), fail_(false) {}

    bool is_eof() const { return eof_; }
    bool is_fail() const { return fail_; }
    
    bool hangup(FDType* fd = 0) {
        return eof_ = true;
    }
    bool fail(FDType* fd = 0) {
        if(fd) ERROR("IO error: " << get_error(fd));
        return fail_ = true;
    }

private:
    bool eof_;
    bool fail_;
};


template<typename FDType>
struct NOP : IOEvent<0>, Control<FDType> {
    typedef void* type;
    NOP(type = 0) {}
    bool perform(FDType*) { return false; }
    type data() { return 0; }
};

class Accept : public Input, public Control<TCPSocket> {
public:
    typedef TCPSocket* type;
    bool perform(TCPSocket* socket) {
        peer_.reset(new TCPSocket(socket->accept()));
        return true;
    }

    type data() { return peer_.release(); }

private:
    std::auto_ptr<TCPSocket> peer_;
};

template<typename FDType>
class Receive : public Input, public Control<FDType> {
public:
    typedef std::pair< std::string, Socket::Target > type;
    bool perform(FDType* socket) {
        char buf[65536];
        // slow TCP clients with TCP_NODELAY not supported yet
        Socket::Request req = socket->read(buf, sizeof(buf));
        if(req.first == 0) return hangup(socket);
        req_ = type(std::string(buf, req.first), req.second);
        return true;
    }

    type data() { return req_; }

private:    
    type req_;
};

template<typename FDType>
class Write : public Output, public Control<FDType> {
    typedef typename boost::function<int (FDType*,const char*,size_t)> Writer;
public:
    typedef std::string type;

    Write(const type& data, Writer writer = &FDType::write) :
        data_(data), pos_(0), writer_(writer) {}

    bool perform(FDType* fd) {
        int pos = writer_(fd, &data_[pos_], data_.size() - pos_);
        if(pos == 0) return hangup(fd);
        pos_ += pos;
        return pos_ == data_.size();
    }

private:
    std::string data_;
    size_t pos_;
    Writer writer_;
};


template<typename F, typename R, typename W> 
struct GetFD {
    static int get(const F* fd) { return fd->get(); }
};

template<typename R>
struct GetFD<Pipe, R, NOP<Pipe> > {
    static int get(const Pipe* pipe) { return pipe->reader().get(); }
};

template<typename W>
struct GetFD<Pipe, NOP<Pipe> , W> {
    static int get(const Pipe* pipe) { return pipe->writer().get(); }
};


#endif // SSERVER_IO_H_INCLUDED
