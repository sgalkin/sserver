#ifndef SSERVER_MESSAGE_H_INCLUDED
#define SSERVER_MESSAGE_H_INCLUDED

#include "fd.h"
#include "log.h"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/function.hpp>
#include <boost/foreach.hpp>
#include <map>
#include <poll.h>

class Message {
public:
    Message(int fd, int events) :
        fd_(fd), events_(events) {}

    virtual ~Message() {}
    
    bool is_complete() const { return false; }

    int fd() const { return fd_; }

    const struct pollfd poll() const {
        struct pollfd pollfd;
        memset(&pollfd, 0, sizeof(pollfd));
        pollfd.fd = fd_;
        pollfd.events = events_;
        return pollfd;
    }

    void read() { DEBUG("read"); do_read(); }
    void write() { DEBUG("write"); do_write(); }
    void hangup() { DEBUG("hangup"); do_hangup(); }
    void error() { DEBUG("error"); do_error(); }
    void pass() {}
    

private:
    virtual void do_read() {}
    virtual void do_write() {}
    virtual void do_hangup() {}
    virtual void do_error() {}

    int fd_;
    int events_;
    // bool complete_;
    // bool eof_;
};

class SocketMessage : public Message {
public:
    SocketMessage(int fd, int events) : Message(fd, events) {}

private:
//    di
};

class AcceptMessage :  public SocketMessage {};
class RequestMessage :  public SocketMessage {};
class ResponseMessage :  public SocketMessage {};


typedef std::map< int, boost::function<void(Message&)> > Handler;
Handler handlers() {
    Handler handler;
    handler.insert(std::make_pair(POLLIN, &Message::read));
    handler.insert(std::make_pair(POLLOUT, &Message::write));
    handler.insert(std::make_pair(POLLERR, &Message::error));
    handler.insert(std::make_pair(POLLHUP, &Message::hangup));
    handler.insert(std::make_pair(0, &Message::pass));
    return handler;
}

class Poller {
    typedef boost::ptr_map<int, Message> Queue;
    typedef std::vector<struct pollfd> Poll;

public:
    void add(Message* msg) {
        std::auto_ptr<Message> message(msg);
        int fd = message->fd();
        std::pair<Queue::const_iterator, bool> result = queue_.insert(fd, message);
        REQUIRE(result.second, "Too many message for one fd(" << fd << ")");
        poll_.push_back(result.first->second->poll());
    }

    Message* get() {
        return 0;
    }

    void perform() {
        while(!(poll_.empty() || poll()));
    }

private:
    bool poll() {
        bool ready = false;
        CHECK_CALL(::poll(&poll_[0], poll_.size(), -1), "poll");
        for(Poll::iterator it = poll_.begin(); it != poll_.end(); ) {
            Message& msg = queue_.at(it->fd);
            handler_.at(it->revents & POLLIN)(msg);
            handler_.at(it->revents & POLLOUT)(msg);
            handler_.at(it->revents & POLLERR)(msg);
            handler_.at(it->revents & POLLHUP)(msg);
            it->revents = 0;
            if(msg.is_complete()) {
                it = poll_.erase(it);
                ready = true;
            } else {
                ++it;
            }
        }
        return ready;
    }

    static const Handler handler_;
    Queue queue_;
    Poll poll_;    
};

const Handler Poller::handler_ = handlers();

#endif // SSERVER_MESSAGE_H_INCLUDED
