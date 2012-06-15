#ifndef SSERVER_SOCKET_H_INCLUDED
#define SSERVER_SOCKET_H_INCLUDED

#include "fd.h"
#include <boost/optional.hpp>
#include <string>
#include <netinet/in.h>

class Socket;
typedef Ref<Socket> SocketRef;

class Socket {
public:
    typedef boost::optional<struct sockaddr_in> Target;
    typedef std::pair<int, Target> Request;

    Socket(SocketRef socket) : socket_(socket.fd) {}
    Socket(Socket& socket) : socket_(socket.socket_.release()) {}
    Socket(int type, const std::string& host, unsigned short port);
    ~Socket();

    Socket& operator=(Socket& socket) {
        Socket(socket).socket_.swap(socket_);
        return *this;
    }

    int socket() const { return socket_.get(); }
    operator SocketRef() { return socket_.release(); }
    int release() { return socket_.release(); }

    std::string error() const;

    int read(char* buf, size_t size, Target::value_type* client);
    int write(const char* buf, size_t size, const Target::value_type* client);

private:
    template<typename Op, typename Data>
    int io(Op operation, Data data, size_t size) {
        return socket_.io(operation, data, size);
    }

    FD socket_;
};

class UDPSocket : private Socket {
public:
    UDPSocket(const std::string& host, unsigned short port) :
        Socket(SOCK_DGRAM, host, port) {}

    int get() const { return socket(); }
    Socket::Request read(char* buf, size_t size);
    int write(const char* buf, size_t size, const Target& client);
    std::string error() const { return Socket::error(); }
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
    Socket::Request read(char* buf, size_t size);
    int write(const char* buf, size_t size, const Target& = boost::none);
    std::string error() const { return Socket::error(); }

    operator SocketRef() { return Socket::operator SocketRef(); }
};


#endif //SSERVER_SOCKET_H_INCLUDED
