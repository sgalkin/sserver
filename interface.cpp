#include "interface.h"
#include "exception.h"
#include "log.h"
#include <boost/utility.hpp>
#include <ifaddrs.h>
#include <netdb.h>

namespace {

class Interfaces : boost::noncopyable {
public:
    Interfaces() {
        CHECK_CALL(getifaddrs(&interfaces_),
                   "Error while reading interfaces:");
    }

    ~Interfaces() {
        freeifaddrs(interfaces_);
    }
    
    operator const struct ifaddrs*() const {
        return interfaces_;
    }

private:
    struct ifaddrs* interfaces_;
};

}

Interface::Interface() {
    Interfaces ifs;
    for(const struct ifaddrs* i = ifs; i; i = i->ifa_next) {
        if(!i->ifa_addr ||
           !(i->ifa_addr->sa_family == AF_INET ||
             false)) // TODO: think about ipv6 not now
//             i->ifa_addr->sa_family == AF_INET6))
            continue;
        char host[NI_MAXHOST];
        CHECK_CALL_ERROR(getnameinfo(i->ifa_addr,
                                     i->ifa_addr->sa_family == AF_INET ?
                                     sizeof(sockaddr_in) : sizeof(sockaddr_in6),
                                     host, sizeof(host), 0, 0,
                                     NI_NUMERICHOST | NI_NUMERICSERV),
                         "Error while processing interface " << i->ifa_name << ":",
                         gai_strerror);
        DEBUG("Found iface: " << i->ifa_name << " -- " << host);
        ifaces_[i->ifa_name] = host;
    }
}

const std::string& Interface::host(const std::string& iface) const {
    IFaces::const_iterator it = ifaces_.find(iface);
    return it == ifaces_.end() ? iface : it->second;
}
