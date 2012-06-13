#ifndef SSERVER_MESSAGE_H_INCLUDED
#define SSERVER_MESSAGE_H_INCLUDED

#include "fd.h"
#include "socket.h"
#include "log.h"
#include "exception.h"
#include "code.h"
#include <boost/scoped_ptr.hpp>
#include <memory>
#include <poll.h>
#include <sys/socket.h>

class MessageBase {
public:
    virtual ~MessageBase() {}
    
//    virtual bool is_done() const = 0;
    virtual bool is_ready() const = 0;
    virtual bool is_eof() const = 0;
    virtual bool is_fail() const = 0;
//    virtual void throw_error() const = 0;

    void read() { DEBUG("read"); do_try(&MessageBase::do_read); }
    void write() { DEBUG("write"); do_try(&MessageBase::do_write); } 
    void hangup() { DEBUG("hangup"); do_hangup(); notify(); }
    void error() { DEBUG("error"); do_error(); notify(); }
    void pass() {}

    virtual int fd() const = 0;
    virtual int events() const = 0;
    virtual void notify() = 0;
    
private:
    void do_try(void (MessageBase::*op)()) {
        try {
            (this->*op)();
            notify();
        } catch(std::runtime_error& ex) {
            ERROR(ex.what());
        }
    }

    virtual void do_read() = 0;
    virtual void do_write() = 0;
    virtual void do_hangup() = 0;
    virtual void do_error() = 0;
//    virtual void do_error(const std::string& error) = 0;
};



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
    
    bool hangup(FDType* fd);

    bool fail(FDType* fd = 0) {
        if(fd) ERROR(get_error(fd));
        fail_ = true;
        return fail_;
    }

private:
    bool eof_;
    bool fail_;
};


template<typename FDType>
struct NOP : IOEvent<0>, Control<FDType> {
    typedef void* type;
    NOP(type = 0) {}
    bool perform(FDType*) { return true; }
    type data() { return 0; }
};

template<typename F, typename R, typename W> 
struct FDHelper { 
    static int get(const F* fd) { return fd->get(); }
};

template<typename R>
struct FDHelper<Pipe, R, NOP<Pipe> > {
    static int get(const Pipe* pipe) { return pipe->reader().get(); }
};

template<typename W>
struct FDHelper<Pipe, NOP<Pipe> , W> {
    static int get(const Pipe* pipe) { return pipe->writer().get(); }
};

template<typename FDType>
inline bool Control<FDType>::hangup(FDType* fd) {
    int f = FDHelper<FDType, Control<FDType>, NOP<FDType> >::get(fd);
    DEBUG("fd(" << f << ") disconnected");
    eof_ = true;
    return eof_;
}


// template<typename FDType>
// class Hangup : public IOEvent<POLLHUP> {
// public:
//     Hangup() : eof_(false) {}
//     void perform(FDType* socket) {
//         int fd = FDHelper<FDType, Hangup<FDType>, NOP<FDType> >::get(socket);
//         DEBUG("fd(" << fd << ") disconnected");
//         eof_ = true; 
//     }
//     bool is_eof() const { return eof_; }

// private:
//     bool eof_;
// };

template<typename FDType> class Accept;

template<>
class Accept<TCPSocket> : public Input, public Control<TCPSocket> {
public:
    typedef TCPSocket* type;
    bool perform(TCPSocket* socket) {
        peer_.reset(new TCPSocket(socket->accept()));
        return true;
    }

    type data() { return peer_.release(); }

private:
    // TODO: auto_ptr is not very good
    std::auto_ptr<TCPSocket> peer_;
};

typedef std::pair< std::string, Socket::Target > SReq;

template<typename FDType>
class Receive : public Input, public Control<FDType> {
public:
    typedef SReq type;
    bool perform(FDType* socket) {
        char buf[65536];
        // slow TCP clients with TCP_NODELAY not supported yet
        Socket::Request req = socket->read(buf, sizeof(buf));
        if(req.first == 0) return hangup(socket);
        //client_ = Client(socket->get(), std::string(buf, req.first), req.second);
        req_ = SReq(std::string(buf, req.first), req.second);
        return true;
    }

    type data() { return req_; }
//    bool is_eof() const { return hangup_.is_eof(); }

private:    
    SReq req_;
//    Hangup<FDType> hangup_;
};

template<typename FDType>
class Penalty : public Input, public Control<FDType> {
public:
    typedef MessageBase* type;
    Penalty() {}
    explicit Penalty(MessageBase* msg) : msg_(msg) {}
    bool perform(FDType* pipe) {
        suppress(*pipe);
        return true;
    }
    type data() { return msg_; }

private:
    // potential memory leak
    MessageBase* msg_;
};

template<typename FDType>
class Echo : public Output, public Control<FDType> {
public:
    typedef SReq type;
    Echo(const SReq& data) :
        data_(data.first), client_(data.second), pos_(0) {}

    bool perform(FDType* socket) {
        int pos = socket->write(&data_[pos_], data_.size() - pos_, client_);
        if(pos == 0) return hangup(socket);
        pos_ += pos;
        return pos_ == data_.size();
    }

private:
    std::string data_;
    Socket::Target client_;
    size_t pos_;
};

template<typename FDType>
class Write : public Output, public Control<FDType> {
public:
    typedef std::string type;
    Write(const type& data) :
        data_(data), pos_(0) {}

    bool perform(FDType* fd) {
        int pos = fd->write(&data_[pos_], data_.size() - pos_);
        if(pos == 0) return hangup(fd);
        pos_ += pos;
        return pos_ == data_.size();
    }

private:
    std::string data_;
    size_t pos_;
};



// template<typename FDType>
// class Error : public IOEvent<POLLERR> {
// public:
//     void perform(FDType* fd) { error_ = get_error(fd); }
//     void perform(const std::string& error) { error_ = error; }
//     operator const std::string& () const { return error_; }
//     void throw_error() const { REQUIRE(error_.empty(), error_); }
// private:
//     std::string error_;
// };

template<typename T> struct Own {};// typedef T type};

template<typename T>
struct Storage {
    typedef T base;
    typedef T* type; 
    static base* get(T* ptr) { return ptr; }
    static const base* get(const T* ptr) { return ptr; }
};

template<typename T>
struct Storage< Own<T> > { 
    typedef T base;
    typedef boost::scoped_ptr<T> type; 
    static base* get(type& ptr) { return ptr.get(); }
    static const base* get(const type& ptr) { return ptr.get(); }
};


template<typename Type,
         template <typename F> class Read = NOP,
         template <typename F> class Write = NOP>
         // template <typename F> class Hangup = Hangup,
         // template <typename F> class Error = Error>
class Message : public MessageBase {
    typedef typename Storage<Type>::base FDType;
public:
    enum { 
        Event = (Read<FDType>::Event | Write<FDType>::Event | POLLHUP | POLLERR)
//                 Hangup<FDType>::Event | Error<FDType>::Event)
    };

    explicit Message(FDType* fd, Pipe* notify = 0,
            const typename Write<FDType>::type& data = typename Write<FDType>::type()) :
        fd_(fd), notify_(notify), writer_(data) {}

    Message(FDType* fd, const Read<FDType>& reader, Pipe* notify = 0) :
        fd_(fd), notify_(notify), reader_(reader) {}

    Message(FDType* fd, const Write<FDType>& writer, Pipe* notify = 0) :
        fd_(fd), notify_(notify), writer_(writer) {}

    Message(FDType* fd, const Read<FDType>& reader, const Write<FDType>& writer, Pipe* notify = 0) :
        fd_(fd), notify_(notify), reader_(reader), writer_(writer) {}

    // in C++11 move data from reader
    // getting data from reader may not be constant
    typename Read<FDType>::type data() { return reader_.data(); }
//    const typename Read<FDType>::type& data() const { return reader_.data(); }

    bool is_ready() const { return ready_ || is_eof() || is_fail(); }
    bool is_eof() const { return reader_.is_eof() || writer_.is_eof(); }
    bool is_fail() const { return reader_.is_fail() || writer_.is_fail(); }
//    void throw_error() const { error_.throw_error(); }

    FDType* get() { return Storage<Type>::get(fd_); }
    int fd() const { return FDHelper<FDType, Read<FDType>, Write<FDType> >::get(Storage<Type>::get(fd_)); }
    int events() const { return Event; }

private:
    void notify() { if(notify_) ::notify(notify_); }

    void do_read() { ready_ = reader_.perform(Storage<Type>::get(fd_)); }
    void do_write() { ready_ = writer_.perform(Storage<Type>::get(fd_)); }
    void do_hangup() {
        reader_.hangup(Storage<Type>::get(fd_));
        writer_.hangup(Storage<Type>::get(fd_));
    }
    void do_error() {
        reader_.fail(Storage<Type>::get(fd_));
        writer_.fail(Storage<Type>::get(fd_));
    }
    //{ error_.perform(Storage<Type>::get(fd_)); }
    //    void do_error(const std::string& error) { error_.perform(error); }

    typename Storage<Type>::type fd_;
    Pipe* notify_;
    Read<FDType> reader_;
    Write<FDType> writer_;
    // Hangup<FDType> hangup_;
    // Error<FDType> error_;

    bool ready_;
};


#endif // SSERVER_MESSAGE_H_INCLUDED
