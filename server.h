#ifndef SSERVER_SERVER_H_INCLUDED
#define SSERVER_SERVER_H_INCLUDED

#include <boost/program_options.hpp>
#include <iostream>

class Server {
public:
    explicit Server(const boost::program_options::variables_map& config) {}
    ~Server() {
        std::cerr << "AAAA" << std::endl;
    }
    void process() { ; }
};

#endif // SSERVER_SERVER_H_INCLUDED
