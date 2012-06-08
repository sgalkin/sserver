#include "test.h"
#include "../log.h"

struct Data {
    Data() { ++count; }
    std::string str() const { return ""; }
    static int count;
};

int Data::count = 0;

BOOST_AUTO_TEST_CASE(test_log_types) {
    DEBUG("test");
    WARN("test");
    ERROR("test");
    FATAL("test");
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
    BOOST_REQUIRE_EQUAL(Data::count, 0);
    DEBUG("debug" << Data().str());
    BOOST_CHECK_EQUAL(Data::count, 0);
    FATAL("fatal" << Data().str());
    BOOST_CHECK_EQUAL(Data::count, 1);
}

// TODO: test_log_fd
