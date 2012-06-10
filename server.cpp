#include "server.h"
#include "interface.h"
#include "log.h"
#include "socket.h"
#include "message.h"
#include <boost/foreach.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <poll.h>
#include <sys/socket.h>

Server::Server(const boost::program_options::variables_map& config) {
    Interface iface;
    tcp_.reset(new TCPSocket(iface.host(config["tcp_if"].as<std::string>()),
                             config["tcp_port"].as<unsigned short>()));
    udp_.reset(new UDPSocket(iface.host(config["udp_if"].as<std::string>()),
                             config["udp_port"].as<unsigned short>()));
}

Server::~Server() {}

void Server::process() {
    boost::ptr_vector<TCPSocket> peers_;
    Poller plr;
    Message<TCPSocket, Accept> accept(tcp_.get());
    Message<UDPSocket, Reader<UDPSocket, Receive<UDPSocket> > > udp_receive(udp_.get());
    plr.add(accept);
    plr.add(udp_receive);
    do {
        plr.perform();
        BOOST_FOREACH(int fd, plr) {
            if(fd == tcp_->get()) {
                accept.throw_error();
                peers_.push_back(accept.data());
                // TODO: Memory leaks here
                Message<TCPSocket, Reader<TCPSocket, Receive<TCPSocket> > >* m = 
                    new Message<TCPSocket, Reader<TCPSocket, Receive<TCPSocket> > >(&peers_.back());
                plr.add(*m);
            } else if(fd == udp_->get()) {
                udp_receive.throw_error();
                DEBUG(udp_receive.data().get<0>());
            }
            // TODO: fast lookup
        }
    } while(true);
}
