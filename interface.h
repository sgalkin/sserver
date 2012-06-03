#ifndef SSERVER_INTERFACE_H_INCLUDED
#define SSERVER_INTERFACE_H_INCLUDED

#include <string>
#include <map>

class Interface {
public:
    Interface();
    const std::string& host(const std::string& iface) const;

private:
    typedef std::map<std::string, std::string> IFaces;
    IFaces ifaces_;
};

#endif //SSERVER_INTERFACE_H_INCLUDED
