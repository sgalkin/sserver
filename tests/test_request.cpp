#include "test.h"
#include "../code.h"
#include "../request.h"

BOOST_AUTO_TEST_CASE(test_request_bad) {
    BOOST_CHECK_THROW(Request(" Req").method(), BadRequest);
    BOOST_CHECK_THROW(Request("rEq").method(), BadRequest);
    BOOST_CHECK_THROW(Request("REQ ").method(), BadRequest);
    BOOST_CHECK_THROW(Request("get").method(), BadRequest);
    BOOST_CHECK_THROW(Request("get foo=\r\n").method(), BadRequest);
    BOOST_CHECK_THROW(Request("get username\r\n").method(), NotAcceptable);
    BOOST_CHECK_THROW(Request("Register username=\r\n").method(), BadRequest);
    BOOST_CHECK_THROW(Request("Register username=;email=\r\n").method(), NotAcceptable);
    BOOST_CHECK_THROW(Request("Register username=;email=;baz=\r\n").method(), BadRequest);
}

BOOST_AUTO_TEST_CASE(test_request_good) {
    Request get("get username=ggg\r\n");
    BOOST_CHECK_EQUAL(get.method(), "GET");
    BOOST_CHECK_EQUAL(get.query("username"), "ggg");
    Request reg("Register username=gggg;email=rrr@example.com\r\n");
    BOOST_CHECK_EQUAL(reg.method(), "REGISTER"); 
    BOOST_CHECK_EQUAL(reg.query("username"), "gggg");
    BOOST_CHECK_EQUAL(reg.query("email"), "rrr@example.com");
}

BOOST_AUTO_TEST_CASE(test_username) {
    BOOST_CHECK(!validate_username(""));
    BOOST_CHECK(!validate_username(" "));
    BOOST_CHECK(!validate_username(" ds"));
    BOOST_CHECK(!validate_username("ds "));
    BOOST_CHECK(validate_username("asd     fds"));
    BOOST_CHECK(validate_username("093     fds  s32=="));
    BOOST_CHECK(validate_username("/+     fds"));
    BOOST_CHECK(validate_username("fo_fds"));
    BOOST_CHECK(validate_username("fo_fd@@s"));
}

BOOST_AUTO_TEST_CASE(test_email) {
    BOOST_CHECK(validate_email("niceandsimple@example.com"));
    BOOST_CHECK(validate_email("simplewithsymbol@example.com"));
    BOOST_CHECK(validate_email("less.common@example.com"));
    BOOST_CHECK(validate_email("a.little.more.unusual@dept.example.com"));
    BOOST_CHECK(validate_email("0@a"));
    BOOST_CHECK(validate_email("!#$%&'*+-/=?^_`{}|~@example.org"));
    BOOST_CHECK(validate_email("postbox@com"));

    BOOST_CHECK(!validate_email("Abc.example.com"));
    BOOST_CHECK(!validate_email("Abc.@example.com"));
    BOOST_CHECK(!validate_email("Abc..123@example.com"));
    BOOST_CHECK(!validate_email("A@b@c@example.com"));
}
