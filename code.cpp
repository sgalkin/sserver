#include "code.h"
#include <boost/format.hpp>
#include <boost/assign.hpp>

namespace {

std::string message(Code code, const std::string& text) {
    static const boost::format frmt("%1% %2%");
    return (boost::format(frmt) % code % text).str();
}

}

std::map<Codes::Code, std::string> Codes::strings_ = boost::assign::map_list_of
    (Codes::OK, message(Codes::OK, "OK"))
    (Codes::BAD_REQUEST, message(Codes::BAD_REQUEST, "Bad request"))
    (Codes::NOT_FOUND, message(Codes::NOT_FOUND, "Not Found"))
    (Codes::OVERLOADED, message(Codes::OVERLOADED, "Overloaded"))
    (Codes::NOT_ACCEPTABLE, message(Codes::NOT_ACCEPTABLE, "Not Acceptable"))
    (Codes::CONFLICT, message(Codes::CONFLICT, "Conflict"))
    (Codes::UNAVAILABLE, message(Codes::UNAVAILABLE, "Service unavailable"))
    .to_container(strings_);
    
