#include "test.h"
#include "helpers.h"
#include "../fd.h"
#include <algorithm>

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

BOOST_AUTO_TEST_CASE(test_write_error) {
    FD full(open("/dev/full", O_RDWR));
    BOOST_CHECK_THROW(full.write("hello", 5), std::runtime_error);
    FD null(open("/dev/null", O_RDONLY));
    BOOST_CHECK_THROW(null.write("hello", 5), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_write_block) {
    int fds[2];
    BOOST_REQUIRE(pipe(&fds[0]) != -1);
    FD r(fds[0]);
    FD w(fds[1]);
    char data[1024 * PIPE_BUF];
    BOOST_CHECK(w.write(data, sizeof(data)) != sizeof(data));
    BOOST_CHECK(w.write(data, sizeof(data)) == 0);
    BOOST_REQUIRE(read(r.get(), &data, sizeof(data)) != -1);
    BOOST_CHECK(w.write(data, sizeof(data)) != 0);
}

BOOST_AUTO_TEST_CASE(test_read_block) {
    int fds[2];
    BOOST_REQUIRE(pipe(&fds[0]) != -1);
    FD r(fds[0]);
    FD w(fds[1]);
    char data[1024 * PIPE_BUF];
    BOOST_CHECK(r.read(data, sizeof(data)) == 0);
    BOOST_REQUIRE(write(w.get(), data, sizeof(data)) != -1);
    BOOST_CHECK(r.read(data, sizeof(data)) != sizeof(data));
    BOOST_CHECK(r.read(data, sizeof(data)) == 0);
}

// Lambdas are only in gcc 4.6 :(
struct Generator {
    char operator()() const {
        static char data = 0;
        return data++;
    }
};

BOOST_AUTO_TEST_CASE(test_io_data) {
    int fds[2];
    BOOST_REQUIRE(pipe(&fds[0]) != -1);
    FD r(fds[0]);
    FD w(fds[1]);
    char sdata[1024 * PIPE_BUF];
    std::generate_n(sdata, sizeof(sdata), Generator());
    char ddata[sizeof(sdata)];
    std::fill_n(ddata, sizeof(ddata), 0);
    BOOST_REQUIRE(memcmp(sdata, ddata, sizeof(sdata)) != 0);
    bool blocks = false;
    char *dpos = ddata;
    BOOST_CHECK(r.read(ddata, sizeof(ddata)) == 0);
    for(const char* spos = sdata; spos != sdata + sizeof(sdata); ) {
        int written = w.write(spos, sdata + sizeof(sdata) - spos);
        BOOST_CHECK(written != -1);
        if(written == 0) {
            blocks = true;
            int read = r.read(dpos, ddata + sizeof(ddata) - dpos);
            BOOST_CHECK(read != -1);
            dpos += read;
        }
        spos += written;
    }
    BOOST_REQUIRE(blocks);
    for( ; dpos != ddata + sizeof(ddata); ) {
        int read = r.read(dpos, ddata + sizeof(ddata) - dpos);
        BOOST_CHECK(read != -1);
        dpos += read;
    }
    BOOST_CHECK(memcmp(sdata, ddata, sizeof(sdata)) == 0);
}

BOOST_AUTO_TEST_CASE(test_pipe) {
    Pipe p;
    char data[] = "hello";
    char buf[sizeof(data)];
    BOOST_REQUIRE(memcmp(data, buf, sizeof(data)) != 0);
    BOOST_CHECK(p.write(data, sizeof(data)) != -1);
    BOOST_CHECK(p.read(buf, sizeof(buf)) != -1);
    BOOST_REQUIRE(memcmp(data, buf, sizeof(data)) == 0);
}
