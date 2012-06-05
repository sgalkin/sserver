#ifndef SSERVER_SOCKET_H_INCLUDED
#define SSERVER_SOCKET_H_INCLUDED

#include "fd.h"
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>


typedef std::pair<int, struct sockaddr_in> Peer;

class Socket {
public:
    Socket(int type, const std::string& host, unsigned short port);
    virtual ~Socket() {};

    int socket() const { return socket_; }

    Peer accept() const;

private:
    virtual int peer(struct sockaddr* addr, socklen_t* addrlen) const = 0;

    FD socket_;
};

class UDPSocket : public /* private */ Socket {
public:
    UDPSocket(const std::string& host, unsigned short port);
private:
    int peer(struct sockaddr* addr, socklen_t* addrlen) const;
};

class TCPSocket : public /* private */ Socket {
public:
    TCPSocket(const std::string& host, unsigned short port);
private:
    int peer(struct sockaddr* addr, socklen_t* addrlen) const;
};

#endif //SSERVER_SOCKET_H_INCLUDED
