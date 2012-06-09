#include "server.h"
#include "interface.h"
#include "log.h"
#include "socket.h"
#include "message.h"
#include <boost/foreach.hpp>
#include <poll.h>
#include <sys/socket.h>

namespace {

struct pollfd make_poll(const Server::Sockets::value_type& s) {
    struct pollfd fd;
    fd.fd = s.first;
    fd.events = POLLIN;
    fd.revents = 0;
    return fd;
}

}

Server::Server(const boost::program_options::variables_map& config) {
    Interface iface;
    std::auto_ptr<Socket> tsocket(
        new TCPSocket(iface.host(config["tcp_if"].as<std::string>()),
                      config["tcp_port"].as<unsigned short>()));
    std::auto_ptr<Socket> usocket(
        new UDPSocket(iface.host(config["udp_if"].as<std::string>()),
                      config["udp_port"].as<unsigned short>()));
    int tfd = tsocket->socket();
    int ufd = usocket->socket();
    sockets_.insert(tfd, tsocket);
    sockets_.insert(ufd, usocket);
}

Server::~Server() {}

void Server::process() {
    Poller plr;
    Message< Reader<Accept> > accept;
    plr.add(sockets_.begin()->first, accept);
    do {
        plr.perform();
        for(int r = plr.get(); r != -1; r = plr.get()) {
            if(r == sockets_.begin()->first) {
                accept.throw_error();
            }
        }
    } while(true);
    // std::vector<struct pollfd> polls;
    // std::transform(sockets_.begin(), sockets_.end(),
    //                std::back_inserter(polls), &make_poll);
    // while(true) {
    //     try {
    //         CHECK_CALL(poll(&polls[0], polls.size(), -1), "poll");
    //         BOOST_FOREACH(const struct pollfd& fd, polls) {
    //             if((fd.revents & POLLERR) == POLLERR) {
    //                 int err;
    //                 socklen_t len = sizeof(err);
    //                 CHECK_CALL(getsockopt(fd.fd, SOL_SOCKET, SO_ERROR, &err, &len),
    //                            "getsockopt(" << fd.fd << ")");
    //                 char dummy;
    //                 THROW(strerror_r(err, &dummy, sizeof(dummy)));
    //             }
    //             if((fd.revents & POLLHUP) == POLLHUP) {
    //                 DEBUG("HUP(" << fd.fd << ")");
    //             }
    //             if((fd.revents & POLLIN) == POLLIN) {
    //                 sockets_.at(fd.fd).accept();
    //             }
    //             if((fd.revents & POLLOUT) == POLLOUT) {
    //                 DEBUG("WRITE(" << fd.fd << ")");
    //             }
    //         }
    //     } catch(std::runtime_error& ex) {
    //         ERROR(ex.what());
    //     }
    // }
}
