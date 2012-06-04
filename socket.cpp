#include "socket.h"
#include "log.h"
#include "exception.h"
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace {

class Address {
public:
    Address(int type, const std::string& host, const std::string& port) {
        struct addrinfo hint;
        memset(&hint, 0, sizeof(hint));
        hint.ai_family = AF_INET; // TODO: thik about IPv6 AF_UNSPEC;
        hint.ai_socktype = type;
        hint.ai_flags = (AI_NUMERICHOST | AI_PASSIVE |
                         AI_NUMERICSERV | AI_ADDRCONFIG);
        CHECK_CALL_ERROR(getaddrinfo(host == "*" ? NULL : host.c_str(),
                                     port.c_str(), &hint, &info_),
                         "Error retriving address information for "
                         << host << ":" << port,
                         gai_strerror);
    }

    ~Address() {
        freeaddrinfo(info_);
    }

    operator const struct addrinfo*() const {
        return info_;
    }

private:
    struct addrinfo* info_;
};

std::string socket_name(const struct sockaddr* sa, socklen_t sz) {
    char host[NI_MAXHOST];
    char port[NI_MAXSERV];
    CHECK_CALL_ERROR(getnameinfo(sa, sz,
                                 host, sizeof(host), port, sizeof(port),
                                 NI_NUMERICHOST | NI_NUMERICSERV),
                     "Couldn't get address name info", gai_strerror);
    return std::string(host) + ":" + std::string(port);
}

inline const std::string& socket_type(int type) {
    static const std::string tcp = "TCP";
    static const std::string udp = "UDP";
    static const std::string unknown = "UNKNOWN";
    if(type == SOCK_DGRAM) return udp;
    if(type == SOCK_STREAM) return tcp;
    return unknown;
}

void unblock(int socket) {
    int flags = 0;
    CHECK_CALL((flags = fcntl(socket, F_GETFL, 0)), "fcntl/GET(" << socket << ")");
    CHECK_CALL(fcntl(socket, F_SETFL, flags | O_NONBLOCK), "fcntl/SET(" << socket << ")");
}

void enable(int socket, int level, int option) {
    int enable = 1;
    CHECK_CALL(setsockopt(socket, level, option, &enable, sizeof(enable)),
               "setsockopt(" << socket << ")");
}

}

Socket::Socket(int type, const std::string& host, unsigned short port) :
    socket_(-1) 
{
    DEBUG("Creating " << socket_type(type) << " socket: " << host << ":" << port);
    Address address(type, host, boost::lexical_cast<std::string>(port));
    const struct addrinfo* info = address;
    int socket;
    CHECK_CALL((socket = ::socket(info->ai_family, info->ai_socktype, info->ai_protocol)),
               "socket (" << socket_type(type) << "): " << host << ":" << port);
    socket_.reset(socket);
    DEBUG("Created " << socket_type(type) << " socket(" << socket_ << "): "
          << socket_name(info->ai_addr, info->ai_addrlen));

    enable(socket_, SOL_SOCKET, SO_REUSEADDR);
    enable(socket_, SOL_SOCKET, SO_KEEPALIVE);
    unblock(socket_);

    CHECK_CALL(bind(socket_, info->ai_addr, info->ai_addrlen), "bind(" << socket_ << ")");

    if(info->ai_next) {
        WARN(host << ":" << port << " has other address info (one of them): "
             << socket_name(info->ai_next->ai_addr, info->ai_next->ai_addrlen));
    }
}

Peer Socket::accept() const {
    struct sockaddr_in addr;
    size_t addrlen = sizeof(addr);
    // Here we are after polling, so no error expected
    int result = peer((struct sockaddr*)&addr, &addrlen);
    REQUIRE(addrlen == sizeof(addr), "strange size"); 
    // REQUIRE(result != -1 || (result == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)), "");
    DEBUG("Accepted(" << socket_ << "): "
          << socket_name((const struct sockaddr*)&addr, addrlen));
    return Peer(result, addr);
}


UDPSocket::UDPSocket(const std::string& host, unsigned short port) :
    Socket(SOCK_DGRAM, host, port) {}

int UDPSocket::peer(struct sockaddr* addr, size_t* size) const {
    CHECK_CALL(recvfrom(socket(), 0, 0, 0, addr, size), "recvfrom(" << socket() << ")");
    return socket();
}


TCPSocket::TCPSocket(const std::string& host, unsigned short port) :
    Socket(SOCK_STREAM, host, port) {
    CHECK_CALL(listen(socket(), 1024), "listen(" << socket() << ")");
}

int TCPSocket::peer(struct sockaddr* addr, size_t* size) const {
    int peer;
    CHECK_CALL((peer = ::accept(socket(), addr, size)), "accept(" << socket() << ")");
    enable(peer, SOL_SOCKET, SO_KEEPALIVE);
    unblock(peer);
    return peer;
}