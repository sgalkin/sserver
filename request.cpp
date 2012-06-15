#include "request.h"
#include "code.h"
#include "log.h"
#include <boost/assign.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>

using namespace boost::assign;

namespace {

typedef std::map<std::string, std::set<std::string> > Methods;

Methods methods() {
    Methods methods;
    std::set<std::string> get = list_of("username").to_container(get);
    std::set<std::string> reg = list_of("username")("email").to_container(reg);
    return map_list_of("GET", get)("REGISTER", reg).to_container(methods);
}

std::pair<std::string, std::string> split(const std::string& str, char sep)
{
    size_t pos = str.find(sep);
    return std::make_pair(str.substr(0, pos),
                          pos == str.npos ? std::string() : str.substr(pos + 1));
}

// boost split modifies string so copy
std::map<std::string, std::string> parse(std::string query) {
    std::map<std::string, std::string> result;
    typedef boost::split_iterator<std::string::iterator> split_iterator;
    for(split_iterator it = boost::make_split_iterator(
            query, boost::first_finder(";", boost::is_iequal()));
        it != split_iterator(); ++it) {
        result.insert(split(boost::copy_range<std::string>(*it), '='));
    }
    return result;
}

}

const Request::Methods Request::methods_ = methods();
const Request::Validate Request::validate_ =
    map_list_of("username", &validate_username)
               ("email", &validate_email).to_container(validate_);

Request::Request(const std::string& request)
{
    DEBUG("Request: " << request.substr(0, request.rfind("\r\n")));
    if(request.rfind("\r\n") != request.size() - 2) throw BadRequest();

    std::string query;
    boost::tie(method_, query) = split(request.substr(0, request.rfind("\r\n")), ' ');
    boost::to_upper(method_);
        
    Methods::const_iterator method = methods_.find(method_);
    if(method == methods_.end()) throw BadRequest();

    query_ = parse(query);
    if(query_.size() != method->second.size()) throw BadRequest();
    BOOST_FOREACH(const Query::value_type& value, query_) {
        if(method->second.find(value.first) == method->second.end()) throw BadRequest();
        Validate::const_iterator validate = validate_.find(value.first);
        if(!validate->second(value.second)) throw NotAcceptable();
    }
}

bool validate_username(const std::string& username) {
    static const boost::regex re("^[[:alnum:]+/](\\ *[[:alnum:][:punct:]])*$");    
    return boost::regex_match(username.begin(), username.end(), re);
}

bool validate_email(const std::string& email) {
    // email address without escapes, quotes and comments
    static const boost::regex re(
        "^[[:alnum:]!#$%&'*+/=?^_`{|}~-](\\.?[[:alnum:]!#$%&'*+/=?^_`{|}~-]+)*@[[:alnum:].-]+$");
    return boost::regex_match(email.begin(), email.end(), re);
}

