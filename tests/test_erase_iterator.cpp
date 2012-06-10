#include "test.h"
#include "../erase_iterator.h"
#include <boost/mpl/list.hpp>

typedef boost::mpl::list< std::list<std::string>, std::vector<std::string> > C;

BOOST_AUTO_TEST_CASE_TEMPLATE(test_equal, collection, C) {
    collection c;
    BOOST_CHECK(erase_iterator<collection>(c) == erase_iterator<collection>());
    BOOST_CHECK(erase_iterator<collection>() == erase_iterator<collection>());
    c.push_back("AAA");
    assert(erase_iterator<collection>(c) == erase_iterator<collection>(c));
    assert(erase_iterator<collection>(c) != erase_iterator<collection>());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase, collection, C) {
    collection c;
    c.push_back("!!!");
    c.push_back("333");
    for(erase_iterator<collection> it(c), end; it != end; ++it);
    BOOST_CHECK(c.empty());
}
