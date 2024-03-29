#ifndef SSERVER_POLL_H_INCLUDED
#define SSERVER_POLL_H_INCLUDED

#include "fd.h"
#include "erase_iterator.h"
#include "message.h"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/foreach.hpp>
#include <list>
#include <vector>
#include <poll.h>

class Poll {
    // Too complex structure
    typedef std::list<MessageBase*> Ready;
    typedef boost::ptr_multimap<int, MessageBase> Queue;
    typedef std::vector<struct pollfd> Pollfd;

public:
    typedef erase_iterator<Ready> iterator;
    typedef void const_iterator; // for BOOST_FOREACH compatibility

    Poll() : outdated_(false) {}

    void add(MessageBase* msg);
    void remove(const MessageBase* msg);

    size_t size() const { return queue_.size(); }

    iterator begin() { return iterator(ready_); }
    iterator end() { return iterator();  } 

    void perform();

private:
    bool poll();

    Queue queue_;
    Pollfd poll_;
    Ready ready_;
    bool outdated_;
};


template<typename FDType>
class Suppress : public Input, public Control<FDType> {
public:
    typedef void* type;
    bool perform(FDType* fd) {
        suppress(fd);
        return true;
    }
    void* data() { return 0; }
};


template<typename Task>
class PollHandler {
public: 
    PollHandler(Pipe* notify) : notify_(notify) {
        poll_.add(new Message< Pipe, Suppress<Pipe> >(notify_));
    }

    virtual ~PollHandler() {};

    virtual void add_task(Task task) = 0;

    virtual void perform() {
        while(true) { 
            fetch_message();
            bool stop = false;
            poll_.perform();
            BOOST_FOREACH(MessageBase* msg, poll_) {
                if(msg->fd() == notify_->reader().get()) {
                    stop = true;
                } else {
                    process_message(msg);
                }
            }
            if(stop) return;
        }
    }

protected:
    Poll poll_;

private:
    virtual void process_message(MessageBase* msg) = 0;
    virtual void fetch_message() {};

    Pipe* notify_;
};

#endif // SSERVER_POLL_H_INCLUDED
