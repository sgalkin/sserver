#include "test.h"
#include "../socket.h"
#include "../log.h"
#include <boost/mpl/list.hpp>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef boost::mpl::list<UDPSocket, TCPSocket> Sockets;

int count_descriptors() {
    int count = 0;
    for(int i = 0; i < sysconf(_SC_OPEN_MAX); ++i) {
        if(fcntl(i, F_GETFL, 0) != -1) ++count;
    }
    return count;
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_socket_invalid, socket, Sockets) {
    set_level(Logger::DEBUG);
    int c = count_descriptors();
    BOOST_CHECK_THROW(socket("123.1.1.1", 3000), std::runtime_error);
    BOOST_CHECK_EQUAL(c, count_descriptors());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_socket_priv, socket, Sockets) {
    if(getuid() != 0) {
        BOOST_CHECK_THROW(socket("127.0.0.1", 64), std::runtime_error);
    } else {
        BOOST_CHECK_NO_THROW(socket("127.0.0.1", 64));
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_socket_all, socket, Sockets) {
    BOOST_CHECK_NO_THROW(socket("*", 10000));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_socket_reopen, socket, Sockets) {
    BOOST_CHECK_NO_THROW(socket("127.0.0.1", 10000));
    BOOST_CHECK_NO_THROW(socket("127.0.0.1", 10000));
    BOOST_CHECK_THROW(socket("123.1.1.1", 3000), std::runtime_error);
    BOOST_CHECK_THROW(socket("123.1.1.1", 3000), std::runtime_error);
}

template<typename S>
struct SocketHelper;

template<>
struct SocketHelper<UDPSocket> {
    enum { type = SOCK_DGRAM };
    static int accept(int fd) { return recvfrom(fd, 0, 0, 0, 0, 0); }
    static int connect(int fd, const struct sockaddr* serv, size_t size) { 
        return sendto(fd, "hello", 6, 0, serv, size); 
    }
};

template<>
struct SocketHelper<TCPSocket> {
    enum { type = SOCK_STREAM };
    static int accept(int fd) { return ::accept(fd, 0, 0); }
    static int connect(int fd, const struct sockaddr* serv, size_t size) {
        return ::connect(fd, serv, size);
    }
};

BOOST_AUTO_TEST_CASE_TEMPLATE(test_socket_init, socket, Sockets) {
    socket s("*", 10000);
    int res = SocketHelper<socket>::accept(s.socket());
    BOOST_CHECK(res == -1 && (errno == EWOULDBLOCK || errno == EAGAIN));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_socket_accept, socket, Sockets) {
    socket s("*", 10000);
    BOOST_CHECK_THROW(s.accept(), std::runtime_error);

    int p = ::socket(AF_INET, SocketHelper<socket>::type, 0);
    BOOST_REQUIRE(p != -1);
    int flags = fcntl(p, F_GETFL, 0);
    BOOST_REQUIRE(flags != -1);
    BOOST_REQUIRE(fcntl(p, F_SETFL, flags | O_NONBLOCK) != -1);

    struct sockaddr_in server;
    size_t size = sizeof(server);
    BOOST_REQUIRE(getsockname(s.socket(), (struct sockaddr*)&server, &size) != -1);

    int cnt = SocketHelper<socket>::connect(p, (const struct sockaddr*)&server, size);
    BOOST_REQUIRE(cnt != -1 || (cnt == -1 &&
                   (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)));

    Peer peer = s.accept();

    struct sockaddr_in client;
    BOOST_REQUIRE(getsockname(p, (struct sockaddr*)&client, &size) != -1);
    if(int(SocketHelper<socket>::type) == SOCK_DGRAM) { // client not bound
        client.sin_addr.s_addr = peer.second.sin_addr.s_addr;
    }
    BOOST_CHECK(memcmp(&peer.second, &client, size) == 0);

    int res = ::recv(peer.first, 0, 0, 0);
    BOOST_CHECK(res != -1 || (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)));
}
