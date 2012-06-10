#ifndef SSERVER_SOCKET_H_INCLUDED
#define SSERVER_SOCKET_H_INCLUDED

#include "fd.h"
#include <boost/tuple/tuple.hpp>
#include <string>
#include <netinet/in.h>

class Socket;
typedef Ref<Socket> SocketRef;

class Socket {
public:
    Socket(SocketRef socket) : socket_(socket.fd) {}
    Socket(Socket& socket) : socket_(socket.socket_.release()) {}
    Socket(int type, const std::string& host, unsigned short port);
    virtual ~Socket();

    Socket& operator=(Socket& socket) {
        Socket(socket).socket_.swap(socket_);
        return *this;
    }

    int socket() const { return socket_.get(); }
    operator SocketRef() { return socket_.release(); }

    template<typename Op, typename Data>
    int io(Op operation, Data data, size_t size) {
        return socket_.io(operation, data, size);
    }

private:
    FD socket_;
};

typedef boost::tuple<std::string, sockaddr_in> UDPMessage;

class UDPSocket : private Socket {
public:
    UDPSocket(const std::string& host, unsigned short port) :
        Socket(SOCK_DGRAM, host, port) {}

    int get() const { return socket(); }
    UDPMessage read(); // TODO: may be use buffers
};

class TCPSocket : private Socket {
public:
    TCPSocket(SocketRef socket) : Socket(socket) {}
    TCPSocket(TCPSocket& socket) : Socket(socket) {}
    TCPSocket(const std::string& host, unsigned short port);

    TCPSocket& operator=(TCPSocket& socket) {
        Socket::operator=(socket);
        return *this; 
    }

    int get() const { return socket(); }

    TCPSocket accept() const;
    std::string read();
    operator SocketRef() { return Socket::operator SocketRef(); }
};

#endif //SSERVER_SOCKET_H_INCLUDED
