#include "server.h"
#include "interface.h"
#include "log.h"
#include "socket.h"
#include "poll.h"
#include <boost/foreach.hpp>

Server::Server(const boost::program_options::variables_map& config) : config_(config) {
    Interface iface;
    tcp_.reset(new TCPSocket(iface.host(config["tcp_if"].as<std::string>()),
                             config["tcp_port"].as<unsigned short>()));
    udp_.reset(new UDPSocket(iface.host(config["udp_if"].as<std::string>()),
                             config["udp_port"].as<unsigned short>()));
}

Server::~Server() {}

void Server::process() {
    Poll poll;

    TCPClient::Accept* accept = new TCPClient::Accept(tcp_.get());
    poll.add(accept);
    UDPClient::Receive* udp_receive = new UDPClient::Receive(udp_.get());
    poll.add(udp_receive);
    do {
        int timeout = config_["sleep"].as<int>();
        std::string datafile = config_["datafile"].as<std::string>();
        poll.perform();
        BOOST_FOREACH(MessageBase* msg, poll) {
            if(msg == udp_receive) {
                udp_pool_.add_task(new UDPClient(udp_receive, datafile, timeout));
            } else if(msg == accept) {
                tcp_pool_.add_task(new TCPClient(accept, datafile, timeout));
            } else {
                ERROR("Unexpected message: fd(" << msg->fd() << ")");
                poll.remove(msg);
            }
        }
    } while(true);
}
