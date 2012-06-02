#include "test.h"
#include "../log.h"

struct Data {
    Data() { std::cerr << "Init" << std::endl; }
    ~Data() { std::cerr << "Destroy" << std::endl; }
    operator std::string() { return "AAAA"; }
};

BOOST_AUTO_TEST_CASE(test_log_types) {
    BOOST_CHECK(false);
}

BOOST_AUTO_TEST_CASE(test_log_invalid_type) {
    BOOST_CHECK(false);
}

BOOST_AUTO_TEST_CASE(test_log_suppression) {
    BOOST_CHECK(false);
}

BOOST_AUTO_TEST_CASE(test_log_pid) {
    BOOST_CHECK(false);
}

BOOST_AUTO_TEST_CASE(test_log_data_init) {
    std::cerr << "AAA" << std::endl;
    DEBUG("daaa" << std::string(Data()));
    std::cerr << "BBB" << std::endl;
    FATAL("12" << std::string(Data()));
    BOOST_CHECK(false);
}

// TODO: test_log_fd
