#ifndef SSERVER_SERVER_H_INCLUDED
#define SSERVER_SERVER_H_INCLUDED

#include "pool.h"
#include <boost/program_options.hpp>
#include <boost/ptr_container/ptr_map.hpp>

class Socket;

class Server {
public:
    typedef boost::ptr_map<int, Socket> Sockets;

    explicit Server(const boost::program_options::variables_map& config);
    ~Server();

    void process();

private:
    Sockets sockets_;
//    Pool pool_;
};

#endif // SSERVER_SERVER_H_INCLUDED
