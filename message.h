#ifndef SSERVER_MESSAGE_H_INCLUDED
#define SSERVER_MESSAGE_H_INCLUDED

#include "fd.h"
#include "log.h"
#include <boost/function.hpp>
#include <boost/foreach.hpp>
#include <map>
#include <poll.h>
#include <sys/socket.h>

class MessageBase {
public:
//    explicit MessageBase() //: fd_(fd) {}
    virtual ~MessageBase() {}
    
    virtual bool is_done() const = 0;
    virtual bool is_ready() const = 0;
    virtual bool is_eof() const = 0;
    virtual bool is_fail() const = 0;
    virtual void throw_error() const = 0;

//    int fd() const { return fd_; }

    void read(int fd) { DEBUG("read"); do_try(fd, &MessageBase::do_read); }
    void write(int fd) { DEBUG("write"); do_try(fd, &MessageBase::do_write); } 
    void hangup(int fd) { DEBUG("hangup"); do_hangup(fd); }
    void error(int fd) { DEBUG("error"); do_error(fd); }
    void pass(int) {}
    
private:
    void do_try(int fd, void (MessageBase::*op)(int)) {
        try {
            (this->*op)(fd); 
        } catch(std::runtime_error& ex) {
            do_error(ex.what());
        }
    }

    virtual void do_read(int fd) = 0;
    virtual void do_write(int fd) = 0;
    virtual void do_hangup(int fd) = 0;
    virtual void do_error(int fd) = 0;
    virtual void do_error(const std::string& error) = 0;

//    int fd_;
};

struct NOP2 {
    typedef void* type;
    enum { Event = 0 };

    explicit NOP2(void *) {}
    bool perform(int) { return true; }
    const type& data() const {
        static const type data = 0;
        return data;
    }
};

struct NOP {
    typedef void* type;
    static bool perform(int, void**) { return true; }
};

#include <netdb.h>

struct Accept {
    typedef int type;
    static bool perform(int fd, int& result) {
        struct sockaddr_in sockaddr;
        socklen_t len = sizeof(sockaddr);
        CHECK_CALL((result = accept(fd, (struct sockaddr*)&sockaddr, &len)), "accept");
// TODO: remove it
        char host[NI_MAXHOST];
        char port[NI_MAXSERV];
        CHECK_CALL_ERROR(getnameinfo((struct sockaddr*)&sockaddr, len,
                                     host, sizeof(host), port, sizeof(port),
                                     NI_NUMERICHOST | NI_NUMERICSERV),
                         "getnameinfo", gai_strerror);
        DEBUG("accept connection from: " << host << ":" << port);
        FD(result).release();
// TODO: set socket options
        return true;
    }
};

template<typename Perform>
class Reader {
public:
    typedef typename Perform::type type;
    enum { Event = POLLIN };

    bool perform(int fd) { return Perform::perform(fd, data_); }
    const type& data() const { return data_; }

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

template<typename DecodeError = DecodeError>
class Error {
public:
    void perform(int fd) { error_ = DecodeError::decode(fd); }
    void perform(const std::string& error) { error_ = error; }
    operator const std::string& () const { return error_; }
    void throw_error() const { if(!error_.empty()) THROW(error_); }
private:
    std::string error_;
};

class Hangup {
public:
    Hangup() : eof_(false) {}
    void perform(int fd) { eof_ = true; }
    operator bool() const { return eof_; }

private:
    bool eof_;
};

template<typename Read = Reader<NOP>,
         typename Write = NOP2,
         typename Hangup = Hangup,
         typename Error = Error<DecodeError> >
class Message : public MessageBase {
public:
    enum { Event = Read::Event | Write::Event };

    Message(const typename Write::type& data = typename Write::type()) : writer_(data) {}
    const typename Read::type& data() const { return reader_.data(); }

    bool is_done() const { return is_eof() || is_fail(); }
    bool is_ready() const { return is_eof() || is_fail() || ready_; }
    bool is_eof() const { return hangup_; }
    bool is_fail() const { return !std::string(error_).empty(); }
    void throw_error() const { error_.throw_error(); }

private:
    void do_read(int fd) { ready_ = reader_.perform(fd); }
    void do_write(int fd) { ready_ = writer_.perform(fd); }
    void do_hangup(int fd) { hangup_.perform(fd); }
    void do_error(int fd) { error_.perform(fd); }
    void do_error(const std::string& error) { error_.perform(error); }

    Read reader_;
    Write writer_;
    Hangup hangup_;
    Error error_;

    bool ready_;
};

template<typename M>
const struct pollfd make_pollfd(int fd, const M&) {
    struct pollfd pollfd;
    memset(&pollfd, 0, sizeof(pollfd));
    pollfd.fd = fd;
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


typedef std::map< int, boost::function<void(MessageBase*,int)> > Handler2;
Handler2 handlers() {
    Handler2 handler;
    handler.insert(std::make_pair(POLLIN, &MessageBase::read));
    handler.insert(std::make_pair(POLLOUT, &MessageBase::write));
    handler.insert(std::make_pair(POLLERR, &MessageBase::error));
    handler.insert(std::make_pair(POLLHUP, &MessageBase::hangup));
    handler.insert(std::make_pair(0, &MessageBase::pass));
    return handler;
}

class Poller {
    typedef std::map<int, MessageBase*> Queue;
    typedef std::vector<struct pollfd> Poll;

public:
    template<typename M>
    void add(int fd, M& msg) {
        REQUIRE(queue_.insert(std::make_pair(fd, &msg)).second,
                "Too many message for one fd(" << fd << ")");
        poll_.push_back(make_pollfd(fd, msg));
    }

    int get() {
        if(ready_.empty()) return -1;
        int fd = ready_.front();
        ready_.pop_front();
        if(queue_.at(fd)->is_done()) queue_.erase(fd);
        return fd;
            
        // TODO: may by done list would be better
        // for(Queue::iterator it = queue_.begin(); it != queue_.end(); ++it) {
        //     if(it->second->is_ready()) {
        //         int fd = it->first;
        //         if(it->second->is_done()) queue_.erase(it);
        //         return fd;
        //     }
        // }
        // return -1;
    }

    void perform() {
        while(!(poll_.empty() || poll()));
    }

private:
    bool poll() {
        CHECK_CALL(::poll(&poll_[0], poll_.size(), -1), "poll");
        for(Poll::iterator it = poll_.begin(); it != poll_.end(); ) {
            MessageBase* msg = queue_.at(it->fd);
            DEBUG("fd(" << it->fd << ") " << it->revents);
            handler_.at(it->revents & POLLIN)(msg, it->fd);
            handler_.at(it->revents & POLLOUT)(msg, it->fd);
            handler_.at(it->revents & POLLERR)(msg, it->fd);
            handler_.at(it->revents & POLLHUP)(msg, it->fd);
            it->revents = 0;
            ready_.push_back(it->fd);
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
    std::list<int> ready_;
};

const Handler2 Poller::handler_ = handlers();

#endif // SSERVER_MESSAGE_H_INCLUDED
