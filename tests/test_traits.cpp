#include "test.h"
#include "../traits.h"
#include <boost/mpl/list.hpp>
#include <boost/type_traits.hpp>
#include <set>
#include <vector>
#include <list>
#include <deque>

typedef boost::mpl::list<char, short, int, long, float, double, long double> Types;

template<typename T, typename IsFloatingPoint = typename boost::is_floating_point<T>::type>
struct add_unsigned;

template<typename T> 
struct add_unsigned<T, boost::true_type> { 
    typedef T type; 
};

template<typename T>
struct add_unsigned<T, boost::false_type> { 
    typedef typename boost::make_unsigned<T>::type type; 
};

template<typename T>
bool check() { return check((IsByteArray<T>*)0); }

template<typename T>
bool check(T* = 0, typename T::type* = 0) { return true; }
bool check(...) { return false; }

BOOST_AUTO_TEST_CASE_TEMPLATE( test_positive, Type,  Types) {
    BOOST_CHECK(check<std::string>());
    BOOST_CHECK_EQUAL(check< std::vector<Type> >(), sizeof(Type) == 1);
    BOOST_CHECK_EQUAL(check< std::vector<typename add_unsigned<Type>::type> >(),
                      sizeof(Type) == 1);
    BOOST_CHECK(!check< std::list<Type> >()); 
    BOOST_CHECK(!check< std::list<typename add_unsigned<Type>::type> >());
    BOOST_CHECK(!check< std::deque<Type> >()); 
    BOOST_CHECK(!check< std::deque<typename add_unsigned<Type>::type> >());
    BOOST_CHECK(!check< std::set<Type> >());
    BOOST_CHECK(!check< std::set<typename add_unsigned<Type>::type> >());
}
