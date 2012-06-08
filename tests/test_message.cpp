#include "test.h"
#include "../message.h"
#include <boost/thread.hpp>

void write(Pipe* pipe) {
    DEBUG("AAA");
    sleep(5);
    pipe->write("*", 1);
    sleep(1);
    close(pipe->writer());
}
#include <iostream>
BOOST_AUTO_TEST_CASE(test_message) {
//    std::cout << poll(0, 0, -1) << std::endl;
    set_level(Logger::DEBUG);
    Poller plr;
    plr.perform();
    Pipe p;
    boost::thread th(boost::bind(&write, &p));
    plr.add(new Message(p.reader(), POLLIN));
    BOOST_CHECK(false);
}
