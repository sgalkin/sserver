#include "socket.h"
#include "log.h"
#include "exception.h"
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
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
        hint.ai_family = AF_INET; // TODO: think about IPv6 AF_UNSPEC;
        hint.ai_socktype = type;
        hint.ai_flags = (AI_NUMERICHOST | AI_PASSIVE |
                         AI_NUMERICSERV | AI_ADDRCONFIG);
        CHECK_CALL_ERROR(getaddrinfo(host == "*" ? NULL : host.c_str(),
                                     port.c_str(), &hint, &info_),
                         "getaddrinfo(" << host << ":" << port << ")", gai_strerror);
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
                     "getnameinfo", gai_strerror);
    return std::string(host) + ":" + std::string(port);
}

std::string peer_name(int fd) {
    Socket::Target::value_type addr;
    socklen_t len = sizeof(addr);
    CHECK_CALL(getpeername(fd, (struct sockaddr*)&addr, &len), "getpeername");
    return socket_name((const sockaddr*)&addr, len);
}

inline const std::string& socket_type(int type) {
    static const std::string tcp = "TCP";
    static const std::string udp = "UDP";
    static const std::string unknown = "UNKNOWN";
    if(type == SOCK_DGRAM) return udp;
    if(type == SOCK_STREAM) return tcp;
    return unknown;
}

void enable(const FD& socket, int level, int option) {
    int enable = 1;
    CHECK_CALL(setsockopt(socket.get(), level, option, &enable, sizeof(enable)),
               "setsockopt(" << socket << ")");
}

}

Socket::Socket(int type, const std::string& host, unsigned short port)
{
  DEBUG("Creating " << socket_type(type) << " socket: " << host << ":" << port);
  Address address(type, host, boost::lexical_cast<std::string>(port));
  const struct addrinfo* info = address;
  socket_.reset(::socket(info->ai_family, info->ai_socktype, info->ai_protocol));
  DEBUG("Created " << socket_type(type) << " socket(" << socket_ << "): "
        << socket_name(info->ai_addr, info->ai_addrlen));

  enable(socket_, SOL_SOCKET, SO_REUSEADDR);
//  enable(socket_, SOL_SOCKET, SO_KEEPALIVE); // server should never close connection

  CHECK_CALL(bind(socket_.get(), info->ai_addr, info->ai_addrlen),
             "bind(" << socket_ << ")");
  
  if(info->ai_next) {
      WARN(host << ":" << port << " has other address info (one of them): "
           << socket_name(info->ai_next->ai_addr, info->ai_next->ai_addrlen));
  }
}

Socket::~Socket() {
    if(socket_.get() != -1) DEBUG("Close socket(" << socket_ << ")");
}

std::string Socket::error() const {
    int err;
    socklen_t len = sizeof(err);
    CHECK_CALL(getsockopt(socket(), SOL_SOCKET, SO_ERROR, &err, &len),
               "getsockopt(" << socket_ << ")");
    return err != 0 ? string_error(err) : "";
}

int Socket::read(char* buf, size_t size, Target::value_type* client) {
    socklen_t len = client ? sizeof(Target::value_type) : 0;
    int read = 0;
    CHECK_CALL((read = io(boost::bind(&recvfrom, _1, _2, _3, 0,
                                      (struct sockaddr*)client, &len), buf, size)),
               "recvfrom(" << socket_ << ")");
    REQUIRE((client == 0 && len == 0) ||
            (client != 0 && len == sizeof(Target::value_type)), "strange address");
    DEBUG("read(" << socket_ << ") from " <<
          (client ? socket_name((const struct sockaddr*)client, len) :
           peer_name(socket_.get())) << " " << read << " byte(s)");
    return read;
}

int Socket::write(const char* buf, size_t size, const Target::value_type* client) {
    socklen_t len = client ? sizeof(Target::value_type) : 0;
    int written = 0;
    CHECK_CALL((written = io(boost::bind(&sendto, _1, _2, _3, 0,
                                         (const struct sockaddr*)client, len), buf, size)),
               "sendto(" << socket_ << ")");
    DEBUG("write(" << socket_ << ") to "
          << (client ? socket_name((const struct sockaddr*)client, len) :
              peer_name(socket_.get())) << " " << written << " byte(s)");
    return written;
}


Socket::Request UDPSocket::read(char* buf, size_t size) {
    Socket::Target::value_type client;
    int read = Socket::read(buf, size, &client);
    return Socket::Request(read, client);
}

int UDPSocket::write(const char* buf, size_t size, const Target& client) {
    REQUIRE(client, "write(" << socket() << "): target not specified");
    return Socket::write(buf, size, &(*client));
}


TCPSocket::TCPSocket(const std::string& host, unsigned short port) :
    Socket(SOCK_STREAM, host, port) {
    CHECK_CALL(listen(socket(), 1024), "listen(" << socket() << ")");
}

TCPSocket TCPSocket::accept() const{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    TCPSocket peer(::accept(socket(), (struct sockaddr*)&addr, &addrlen));
    REQUIRE(addrlen == sizeof(addr), "strange size");
    DEBUG("Accepted(" << socket() << "): "
          << socket_name((const struct sockaddr*)&addr, addrlen)
          << " as fd(" << peer.socket() << ")");
    return peer;
}

Socket::Request TCPSocket::read(char* buf, size_t size) {
    return Socket::Request(Socket::read(buf, size, 0), boost::none);
}

int TCPSocket::write(const char* buf, size_t size, const Target&) {
    return Socket::write(buf, size, 0);
}
