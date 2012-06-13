#ifndef SSERVER_SERVER_H_INCLUDED
#define SSERVER_SERVER_H_INCLUDED

#include "socket.h"
#include "pool.h"
#include "client.h"
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
    Pool<TCPClient*, ClientHandler<TCPClient*> > tcp_pool_;
    Pool<UDPClient*, ClientHandler<UDPClient*> > udp_pool_;

    boost::program_options::variables_map config_;
};

#endif // SSERVER_SERVER_H_INCLUDED
