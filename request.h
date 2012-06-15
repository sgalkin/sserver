#ifndef SSERVER_REQUEST_H_INCLUDED
#define SSERVER_REQUEST_H_INCLUDED

#include <boost/function.hpp>
#include <string>
#include <map>
#include <set>

bool validate_username(const std::string& username);
bool validate_email(const std::string& email);

class Request {
    typedef std::map<std::string, std::string> Query;
    typedef std::map<std::string, std::set<Query::mapped_type> > Methods;    
    typedef std::map<std::string, boost::function<bool (const Query::mapped_type&)> > Validate;

public:
    explicit Request(const std::string& data);

    const std::string& method() const { return method_; }
    const Query::mapped_type& query(const Query::key_type& name) const {
        return query_.at(name);
    }

private:
    std::string method_;
    Query query_;

    static const Methods methods_;
    static const Validate validate_;
};

#endif // SSERVER_REQUEST_H_INCLUDED
