#include "test.h"
#include "../interface.h"

BOOST_AUTO_TEST_CASE(test_ifaces) {
    Interface iface;
    BOOST_CHECK_EQUAL(iface.host("lo"), "127.0.0.1");
    BOOST_CHECK_EQUAL(iface.host("*"), "*");
    BOOST_CHECK_EQUAL(iface.host("129.123.423.11"), "129.123.423.11");
}
