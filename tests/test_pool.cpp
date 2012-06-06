#include "test.h"
#include "../pool.h"
#include "../fd.h"

struct Perform {
    void add(int) {}
    void perform(Pipe&) {}
};

BOOST_AUTO_TEST_CASE( test_pool ) {
    set_level(Logger::DEBUG);
    Pool<int, Perform> p;    
    BOOST_CHECK(false);
}
