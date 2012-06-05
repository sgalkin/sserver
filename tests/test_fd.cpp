#include "test.h"
#include "helpers.h"
#include "../fd.h"

BOOST_AUTO_TEST_CASE(test_construct) {
    int c = count_descriptors();
    {
        int f = open("/dev/null", O_RDONLY);
        BOOST_REQUIRE(f != -1);
        FD fd(f);
        BOOST_CHECK_EQUAL(count_descriptors(), c + 1);
        BOOST_CHECK_EQUAL(fd.get(), f);
    }
    BOOST_CHECK_EQUAL(count_descriptors(), c);
}

BOOST_AUTO_TEST_CASE(test_get) {
    FD fd(1024);
    BOOST_CHECK_EQUAL(fd.get(), 1024);
    BOOST_CHECK_EQUAL(fd, 1024);
}

BOOST_AUTO_TEST_CASE(test_reset_empty) {
    int c = count_descriptors();
    int s = open("/dev/null", O_RDONLY);
    BOOST_REQUIRE(s != -1);
    FD fd(s);
    fd.reset();
    BOOST_CHECK_EQUAL(count_descriptors(), c);
    BOOST_CHECK_EQUAL(fd.get(), -1);
}

BOOST_AUTO_TEST_CASE(test_reset_fd) {
    int s = open("/dev/null", O_RDONLY);
    BOOST_REQUIRE(s != -1);
    int c = count_descriptors();
    FD fd(s);
    int d = open("/dev/null", O_RDONLY);
    BOOST_REQUIRE(d != -1);
    BOOST_CHECK_EQUAL(count_descriptors(), c + 1);
    fd.reset(d);
    BOOST_CHECK_EQUAL(count_descriptors(), c);
    BOOST_CHECK_EQUAL(fd.get(), fd);
}

BOOST_AUTO_TEST_CASE(test_release) {
    int s = open("/dev/null", O_RDONLY);
    BOOST_REQUIRE(s != -1);
    int c = count_descriptors();
    {
        FD f(s);
        BOOST_CHECK_EQUAL(count_descriptors(), c);
        BOOST_CHECK_EQUAL(s, f.release());
        BOOST_CHECK_EQUAL(count_descriptors(), c);
        BOOST_CHECK_EQUAL(-1, f.get());
    }
    BOOST_CHECK_EQUAL(count_descriptors(), c);
    close(s);
}

BOOST_AUTO_TEST_CASE(test_options) {
    FD fd(open("/dev/null", O_RDONLY));
    BOOST_REQUIRE(fd != -1);
    int flags = 0;
    BOOST_REQUIRE((flags = fcntl(fd.get(), F_GETFL, &flags)) != -1);
    BOOST_REQUIRE((flags & O_NONBLOCK) != O_NONBLOCK);
    fd.set_nonblock();
    BOOST_CHECK((flags = fcntl(fd.get(), F_GETFL, &flags)) != -1);
    BOOST_CHECK_EQUAL((flags & O_NONBLOCK), O_NONBLOCK);
}

BOOST_AUTO_TEST_CASE(test_swap) {
    int src = open("/dev/null", O_RDONLY);
    int dst = open("/dev/null", O_RDONLY);
    BOOST_REQUIRE(src != -1 || dst != -1);
    int c = count_descriptors();
    FD s(src);
    FD d(dst);
    swap(s, d);
    BOOST_CHECK_EQUAL(count_descriptors(), c);
    BOOST_CHECK_EQUAL(src, d.get());
    BOOST_CHECK_EQUAL(dst, s.get());
}
