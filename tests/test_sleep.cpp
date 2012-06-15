#include "test.h"
#include "../sleep.h"
#include "../log.h"
#include "../pool.h"
#include <sys/poll.h>

struct pollfd make_poll(const Pipe& p) {
    struct pollfd pollfd;
    pollfd.fd = p.reader().get();
    pollfd.events = POLLIN;
    pollfd.revents = 0;
    return pollfd;
}

BOOST_AUTO_TEST_CASE( test_sleep ) {
    Pool<Sleep::Client, Sleep> pool(1);
    Pipe p1;
    Pipe p2;
    time_t s = time(0);
    pool.add_task(Sleep::Client(&p1, 2000));
    sleep(1);
    pool.add_task(Sleep::Client(&p2, 3000));
    typedef std::vector<struct pollfd> Pollfd;
    Pollfd pollfd;
    pollfd.push_back(make_poll(p1));
    pollfd.push_back(make_poll(p2));
    while(!pollfd.empty()) {
        BOOST_REQUIRE(poll(&pollfd[0], pollfd.size(), -1) != -1);
        for(Pollfd::iterator it = pollfd.begin(); it != pollfd.end(); ) {
            if(it->revents == 0) { ++it; continue; }
            if(it->fd == p1.reader().get()) {
                BOOST_CHECK(time(0) - s >= 1 && time(0) - s <= 3);
            } else {
                BOOST_CHECK(time(0) - s >= 4 && time(0) - s <= 6);
            }
            it = pollfd.erase(it);
        }
    }
}
