#include "sleep.h"
#include "fd.h"
#include "code.h"
#include "exception.h"
#include <sys/select.h>

void Sleep::add_task(Client client) {
    if(client.second <= 0) notify(client.first);
    else clients_.push_back(client);
}

void Sleep::perform() {
    while(true) {
        fd_set in;
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1000; // 1 millisecond
        FD_ZERO(&in);
        FD_SET(pipe_->reader().get(), &in);  // pool notification pin
        int res = 0;
        CHECK_CALL((res = select(pipe_->reader().get() + 1,
                                 &in, 0, 0, clients_.empty() ? 0 : &tv)), "select");
        if(res != 0) {
            suppress(pipe_);
            break; // can lose 1 millisecond hera, but not a big problem
        }
        for(Clients::iterator it = clients_.begin(); it != clients_.end(); ) {
            if(--(it->second) <= 0) {
                notify(it->first);
                it = clients_.erase(it);
            } else {
                ++it;
            }
        }
    }
}
