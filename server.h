#ifndef SSERVER_SERVER_H_INCLUDED
#define SSERVER_SERVER_H_INCLUDED

#include "socket.h"
#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

class TCPSocket;
class UDPSocket;

class Server {
public:
    explicit Server(const boost::program_options::variables_map& config);
    ~Server();

    void process();

private:
    boost::scoped_ptr<TCPSocket> tcp_;
    boost::scoped_ptr<UDPSocket> udp_;
};

#endif // SSERVER_SERVER_H_INCLUDED
