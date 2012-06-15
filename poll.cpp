#include "poll.h"
#include "exception.h"
#include "log.h"
#include "message.h"

namespace {

inline struct pollfd make_pollfd(const MessageBase* msg) {
    struct pollfd pollfd;
    memset(&pollfd, 0, sizeof(pollfd));
    pollfd.fd = msg->fd();
    pollfd.events = msg->events();
    return pollfd;
}

}

void Poll::add(MessageBase* msg) {
    if(msg == 0) return;
    int fd = msg->fd();
    Queue::iterator result = queue_.insert(fd, msg);
    poll_.push_back(make_pollfd(result->second));
}

void Poll::remove(const MessageBase* msg) {
    if(msg == 0) return;
    for(Pollfd::iterator it = poll_.begin(); it != poll_.end(); ++it) {
        if(it->fd == msg->fd() && it->events == msg->events()) {
            poll_.erase(it);
            break;
        }
    }

    boost::iterator_range<Queue::iterator> range = queue_.equal_range(msg->fd());
    for(Queue::iterator it = range.begin(); it != range.end(); ++it) {
        if(it->second == msg) {
            it = queue_.erase(it);
            outdated_ = true;
            break;
        }
    }
}

void Poll::perform() { 
    if(outdated_) {
        Pollfd poll;
        for(Queue::const_iterator it = queue_.begin(); it != queue_.end(); ++it) {
            poll.push_back(make_pollfd(it->second));
        }
        poll_.swap(poll);
        outdated_ = false;
    }
    while(!(poll_.empty() || poll())); 
}

bool Poll::poll() {
    int ready = 0;
    CHECK_CALL((ready = ::poll(&poll_[0], poll_.size(), -1)), "poll");
    for(Pollfd::iterator it = poll_.begin(); it != poll_.end() && ready > 0; ++it) {
        if(it->revents == 0) continue;
        --ready;
        BOOST_FOREACH(Queue::value_type v, queue_.equal_range(it->fd)) {
            MessageBase& msg = *v.second;
            if((it->revents & POLLERR) == POLLERR) msg.error();
            if((it->revents & POLLHUP) == POLLHUP) msg.hangup();
            if((it->revents & POLLIN) == POLLIN) msg.read();
            if((it->revents & POLLOUT) == POLLOUT) msg.write();
            if(msg.is_ready()) ready_.push_back(&msg);
        }
        it->revents = 0;
    }
    return !ready_.empty();
}
