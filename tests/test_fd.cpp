#include "test.h"
#include "helpers.h"
#include "../fd.h"

BOOST_AUTO_TEST_CASE(test_construct_empty) {
    int c = count_descriptors();
    {
        FD fd;
        BOOST_CHECK_EQUAL(count_descriptors(), c);
        BOOST_CHECK_EQUAL(fd, -1);
    }
    BOOST_CHECK_EQUAL(count_descriptors(), c);
}

BOOST_AUTO_TEST_CASE(test_construct_fd) {
    int c = count_descriptors();
    {
        int f = open("/dev/null", O_RDONLY);
        int flags = 0;
        BOOST_REQUIRE(f != -1);
        BOOST_REQUIRE_EQUAL(count_descriptors(), c + 1);
        BOOST_REQUIRE((flags = fcntl(f, F_GETFL, &flags)) != -1);
        BOOST_REQUIRE((flags & O_NONBLOCK) != O_NONBLOCK);
        FD fd(f);
        BOOST_CHECK_EQUAL(count_descriptors(), c + 1);
        BOOST_CHECK_EQUAL(fd.get(), f);
        BOOST_CHECK((flags = fcntl(fd.get(), F_GETFL, &flags)) != -1);
        BOOST_CHECK_EQUAL((flags & O_NONBLOCK), O_NONBLOCK);
    }
    BOOST_CHECK_EQUAL(count_descriptors(), c);
}

BOOST_AUTO_TEST_CASE(test_get) {
    FD fd(1024, false);
    BOOST_CHECK_EQUAL(fd.get(), 1024);
    BOOST_CHECK_EQUAL(fd, 1024);
}

BOOST_AUTO_TEST_CASE(test_reset_empty) {
    int c = count_descriptors();
    FD fd(open("/dev/null", O_RDONLY));
    BOOST_CHECK_EQUAL(count_descriptors(), c + 1);
    fd.reset();
    BOOST_CHECK_EQUAL(count_descriptors(), c);
    BOOST_CHECK_EQUAL(fd.get(), -1);
}

BOOST_AUTO_TEST_CASE(test_reset_fd) {
    FD fd(open("/dev/null", O_RDONLY));
    int c = count_descriptors();
    int d = open("/dev/null", O_RDONLY);
    BOOST_REQUIRE(d != -1);
    BOOST_REQUIRE_EQUAL(count_descriptors(), c + 1);
    fd.reset(d);
    BOOST_CHECK_EQUAL(count_descriptors(), c);
    BOOST_CHECK_EQUAL(fd.get(), d);
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

BOOST_AUTO_TEST_CASE(test_swap) {
    FD s(open("/dev/null", O_RDONLY));
    FD d(open("/dev/null", O_RDONLY));
    int c = count_descriptors();
    int src = s;
    int dst = d;
    swap(s, d);
    BOOST_CHECK_EQUAL(count_descriptors(), c);
    BOOST_CHECK_EQUAL(src, d.get());
    BOOST_CHECK_EQUAL(dst, s.get());
    s.swap(d);
    BOOST_CHECK_EQUAL(count_descriptors(), c);
    BOOST_CHECK_EQUAL(dst, d.get());
    BOOST_CHECK_EQUAL(src, s.get());
}
