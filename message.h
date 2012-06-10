#ifndef SSERVER_MESSAGE_H_INCLUDED
#define SSERVER_MESSAGE_H_INCLUDED

#include "fd.h"
#include "socket.h"
#include "log.h"
#include "exception.h"
#include "erase_iterator.h"
#include <boost/function.hpp>
#include <boost/foreach.hpp>
#include <boost/iterator.hpp>
//#include <boost/scoped_ptr.hpp>
#include <map>
#include <list>
#include <poll.h>
#include <sys/socket.h>

class MessageBase {
public:
    virtual ~MessageBase() {}
    
    virtual bool is_done() const = 0;
    virtual bool is_ready() const = 0;
    virtual bool is_eof() const = 0;
    virtual bool is_fail() const = 0;
//    virtual void throw_error() const = 0;

    void read() { DEBUG("read"); do_try(&MessageBase::do_read); }
    void write() { DEBUG("write"); do_try(&MessageBase::do_write); } 
    void hangup() { DEBUG("hangup"); do_hangup(); }
    void error() { DEBUG("error"); do_error(); }
    void pass() {}

    virtual int fd() const = 0;
    
private:
    void do_try(void (MessageBase::*op)()) {
        try {
            (this->*op)();
        } catch(std::runtime_error& ex) {
            do_error(ex.what());
        }
    }

    virtual void do_read() = 0;
    virtual void do_write() = 0;
    virtual void do_hangup() = 0;
    virtual void do_error() = 0;
    virtual void do_error(const std::string& error) = 0;
};

template<typename T>
struct NOP2 {
    typedef void* type;
    enum { Event = 0 };

    explicit NOP2(void *) {}
    bool perform(T*) { return true; }
    const type& data() const {
        static const type data = 0;
        return data;
    }
};

template<typename T>
struct NOP {
    typedef void* type;
    static bool perform(T*, type&) { return true; }
};

class Accept {
public:
    typedef TCPSocket* type;
    enum { Event = POLLIN };

    bool perform(TCPSocket* socket) {
        peer_.reset(new TCPSocket(socket->accept()));
        return true;
    }

    type data() {
        return peer_.release();
    }

private:
    std::auto_ptr<TCPSocket> peer_; // TODO: think here a little
};

template<typename T> struct Receive;

template<> 
struct Receive<UDPSocket> {
    typedef UDPMessage type;
    static bool perform(UDPSocket* socket, UDPMessage& message) {
        message = socket->read();
        return true;
    }
};

template<> 
struct Receive<TCPSocket> {
    typedef std::string type;
    static bool perform(TCPSocket* socket, std::string& message) {
        message = socket->read();
        // TODO: handle slow clients with TCP_NODELAY
        DEBUG(socket->get() << " READ: " << message);
        return true;
        // return (message.size() > 1 &&
        //         message[message.size() - 1] = '\n' &&
        //         message[message.size() - 2] = '\r');
    }
};

template<typename T, typename Perform>
class Reader {
public:
    typedef typename Perform::type type;
    enum { Event = POLLIN };

    bool perform(T* fd) { return Perform::perform(fd, data_); }
    type data() const { return data_; }

private:
    type data_;
};

struct DecodeSocketError {
    static std::string decode(int fd) {
        int err;
        socklen_t len = sizeof(err);
        CHECK_CALL(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len),
                   "getsockopt(" << fd << ")");
        char dummy;
        return strerror_r(err, &dummy, sizeof(dummy));
    }
};

struct DecodeError {
    static std::string decode(int fd) {
        std::stringstream s;
        s << "Unexpected error fd(" << fd << ")";
        return s.str();
    }
};

template<typename T, typename DecodeError = DecodeError>
class Error {
public:
    void perform(T* fd) { error_ = DecodeError::decode(fd->get()); }
    void perform(const std::string& error) { error_ = error; }
    operator const std::string& () const { return error_; }
    void throw_error() const { if(!error_.empty()) THROW(error_); }
private:
    std::string error_;
};

template<typename T>
class Hangup {
public:
    Hangup() : eof_(false) {}
    void perform(T*) { eof_ = true; }
    operator bool() const { return eof_; }

private:
    bool eof_;
};


template<typename FDType,
         typename Read = Reader<FDType, NOP<FDType> >,
         typename Write = NOP2<FDType>,
         typename Hangup = Hangup<FDType>,
         typename Error = Error<FDType,DecodeError>,
         bool Finite = false>
class Message : public MessageBase {
public:
    enum { Event = Read::Event | Write::Event };

    Message(FDType* fd,
            const typename Write::type& data = typename Write::type()) :
        fd_(fd), writer_(data) {}

    // in C++11 move data from reader
    // getting data from reader may not be constant
    typename Read::type data() { return reader_.data(); }

    bool is_done() const { return Finite && is_ready(); }
    bool is_ready() const { return ready_ || is_eof() || is_fail(); }
    bool is_eof() const { return hangup_; }
    bool is_fail() const { return !std::string(error_).empty(); }
    void throw_error() const { error_.throw_error(); }

    int fd() const { return fd_->get(); }

private:
    void do_read() { ready_ = reader_.perform(fd_); }
    void do_write() { ready_ = writer_.perform(fd_); }
    void do_hangup() { hangup_.perform(fd_); }
    void do_error() { error_.perform(fd_); }
    void do_error(const std::string& error) { error_.perform(error); }

    FDType* fd_;

    Read reader_;
    Write writer_;
    Hangup hangup_;
    Error error_;

    bool ready_;
};

template<typename M>
const struct pollfd make_pollfd(const M& msg) {
    struct pollfd pollfd;
    memset(&pollfd, 0, sizeof(pollfd));
    pollfd.fd = msg.fd();
    pollfd.events = M::Event;
    return pollfd;
}


// class SocketMessage : public Message {
// public:
//     SocketMessage(int fd, int events) : Message(fd, events) {}

// private:
// //    di
// };

// class AcceptMessage :  public SocketMessage {};
// class RequestMessage :  public SocketMessage {};
// class ResponseMessage :  public SocketMessage {};


typedef std::map< int, boost::function<void(MessageBase*)> > Handler2;
inline Handler2 handlers() {
    Handler2 handler;
    handler.insert(std::make_pair(POLLIN, &MessageBase::read));
    handler.insert(std::make_pair(POLLOUT, &MessageBase::write));
    handler.insert(std::make_pair(POLLERR, &MessageBase::error));
    handler.insert(std::make_pair(POLLHUP, &MessageBase::hangup));
    handler.insert(std::make_pair(0, &MessageBase::pass));
    return handler;
}

class Poller {
    typedef std::list<int> Ready;
    typedef std::map<int, MessageBase*> Queue;
    typedef std::vector<struct pollfd> Poll;

public:
    typedef erase_iterator<Ready> iterator;
    typedef void const_iterator; // for BOOST_FOREACH compatibility

    template<typename M>
    void add(M& msg) {
        REQUIRE(queue_.insert(std::make_pair(msg.fd(), &msg)).second,
                "Too many message for one fd(" << msg.fd() << ")");
        poll_.push_back(make_pollfd(msg));
    }

    // TODO: how to erase done tasks?
    iterator begin() { return iterator(ready_); }
    iterator end() { return iterator();  } 

    // int get() {
    //     if(ready_.empty()) return -1;
    //     int fd = ready_.front();
    //     ready_.pop();
    //     if(queue_.at(fd)->is_done()) queue_.erase(fd);
    //     return fd;
            
        // TODO: may by done list would be better
        // for(Queue::iterator it = queue_.begin(); it != queue_.end(); ++it) {
        //     if(it->second->is_ready()) {
        //         int fd = it->first;
        //         if(it->second->is_done()) queue_.erase(it);
        //         return fd;
        //     }
        // }
        // return -1;
//    }

    void perform() {
        while(!(poll_.empty() || poll()));
    }

private:
    bool poll() {
        int ready = 0;
        CHECK_CALL((ready = ::poll(&poll_[0], poll_.size(), -1)), "poll");
        for(Poll::iterator it = poll_.begin(); it != poll_.end() && ready > 0; ) {
            MessageBase* msg = queue_.at(it->fd);
            if(it->revents == 0) {
                ++it;
                continue;
            }
            --ready;
            handler_.at(it->revents & POLLERR)(msg);
            handler_.at(it->revents & POLLHUP)(msg);
            handler_.at(it->revents & POLLIN)(msg);
            handler_.at(it->revents & POLLOUT)(msg);
            it->revents = 0;
            if(msg->is_ready()) ready_.push_back(it->fd);
            if(msg->is_done()) {
                it = poll_.erase(it);
            } else {
                ++it;
            }
        }
        return !ready_.empty();
    }

    static const Handler2 handler_;
    Queue queue_;
    Poll poll_;
    Ready ready_;
};

const Handler2 Poller::handler_ = handlers();

#endif // SSERVER_MESSAGE_H_INCLUDED
