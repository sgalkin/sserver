#include "test.h"
#include "../interface.h"
#include <ifaddrs.h>

std::string loopback() {
    struct ifaddrs* addrs;
    if(getifaddrs(&addrs) == -1 || !addrs) return "";
    std::string lo;
    for(struct ifaddrs* i = addrs; i != NULL; i = i->ifa_next) {
        if(strstr(addrs->ifa_name, "lo") == addrs->ifa_name) {
            lo = addrs->ifa_name;
            break;
        }
    }
    freeifaddrs(addrs);
    return lo;
}

BOOST_AUTO_TEST_CASE(test_ifaces) {
    std::string lo = loopback();
    BOOST_REQUIRE(!lo.empty());
    Interface iface;
    BOOST_CHECK_EQUAL(iface.host(lo), "127.0.0.1");
    BOOST_CHECK_EQUAL(iface.host("*"), "*");
    BOOST_CHECK_EQUAL(iface.host("129.123.423.11"), "129.123.423.11");
}
