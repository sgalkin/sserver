#include "test.h"
#include "../file.h"

BOOST_AUTO_TEST_CASE( test_throw ) {
    BOOST_CHECK_THROW(File("nonexistent", O_RDONLY), std::runtime_error);
    BOOST_CHECK_THROW(File("/etc/shadow", O_RDONLY), std::runtime_error);
}

BOOST_AUTO_TEST_CASE( test_size ) {
    File f("tests/test.dat", O_RDONLY);
    BOOST_CHECK_EQUAL(f.size(), 98379);
}

BOOST_AUTO_TEST_CASE( test_record ) {
    Record rcd0("");
    BOOST_CHECK_EQUAL(rcd0.name(), "");
    BOOST_CHECK_EQUAL(rcd0.email(), "");

    Record rcd1("aaaa");
    BOOST_CHECK_EQUAL(rcd1.name(), "aaaa");
    BOOST_CHECK_EQUAL(rcd1.email(), "");

    Record rcd2(";aaaa");
    BOOST_CHECK_EQUAL(rcd2.name(), "");
    BOOST_CHECK_EQUAL(rcd2.email(), "aaaa");

    Record rcd3("bbbb;aaaa");
    BOOST_CHECK_EQUAL(rcd3.name(), "bbbb");
    BOOST_CHECK_EQUAL(rcd3.email(), "aaaa");
}
