#include "poll.h"
#include "exception.h"
#include "log.h"
#include "message.h"

namespace {

typedef std::map< int, boost::function<void(MessageBase&)> > Handler;

inline Handler handlers() {
    Handler handler;
    handler.insert(std::make_pair(POLLIN, &MessageBase::read));
    handler.insert(std::make_pair(POLLOUT, &MessageBase::write));
    handler.insert(std::make_pair(POLLERR, &MessageBase::error));
    handler.insert(std::make_pair(POLLHUP, &MessageBase::hangup));
    handler.insert(std::make_pair(0, &MessageBase::pass));
    return handler;
}

inline struct pollfd make_pollfd(const MessageBase* msg) {
    struct pollfd pollfd;
    memset(&pollfd, 0, sizeof(pollfd));
    pollfd.fd = msg->fd();
    pollfd.events = msg->events();
    return pollfd;
}

}

const Poll::Handler Poll::handler_ = handlers();

void Poll::add(MessageBase* msg) {
    int fd = msg->fd();
    Queue::iterator result = queue_.insert(fd, msg);
//        std::pair<Queue::iterator, bool> result = queue_.insert(fd, msg);
//        REQUIRE(result.second, "Too many message for one fd(" << fd << ")");
    poll_.push_back(make_pollfd(result/*.first*/->second));
}

void Poll::remove(const MessageBase* msg) {
    // TODO: slow lookup
    for(Pollfd::iterator it = poll_.begin(); it != poll_.end(); ++it) {
        if(it->fd == msg->fd() && it->events == msg->events()) {
            poll_.erase(it);
            break;
        }
    }
    //if(queue_.erase(msg->fd());
    boost::iterator_range<Queue::iterator> range = queue_.equal_range(msg->fd());
    for(Queue::iterator it = range.begin(); it != range.end(); ++it) {
        if(it->second == msg) {
            queue_.erase(it); 
            break;
        }
    }
}

bool Poll::poll() {
    int ready = 0;
    CHECK_CALL((ready = ::poll(&poll_[0], poll_.size(), -1)), "poll");
    for(Pollfd::iterator it = poll_.begin(); it != poll_.end() && ready > 0; ++it) {
        if(it->revents == 0) continue;
        BOOST_FOREACH(Queue::value_type v, queue_.equal_range(it->fd)) {
            if(v.second->events() != it->events) continue;
//                MessageBase& msg = queue_.at(it->fd); //*v.second;
            MessageBase& msg = *v.second;
            --ready;
            handler_.at(it->revents & POLLERR)(msg);
            handler_.at(it->revents & POLLHUP)(msg);
            handler_.at(it->revents & POLLIN)(msg);
            handler_.at(it->revents & POLLOUT)(msg);
            it->revents = 0;
            if(msg.is_ready()) ready_.push_back(&msg);
        }
    }
    return !ready_.empty();
}
