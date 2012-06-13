#include "test.h"
#include "helpers.h"
#include "../socket.h"
#include "../log.h"
#include <boost/mpl/list.hpp>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef boost::mpl::list<UDPSocket, TCPSocket> Sockets;


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
    static int touch(int fd) { return recv(fd, 0, 0, 0); }
    // static int accept(int) { return 0; }
    // static int connect(int, const struct sockaddr*, socklen_t) { return 0; }
    // static int send(int fd,
    //                 const struct sockaddr* addr, socklen_t len, 
    //                 const std::string& data) {
    //     return sendto(fd, data.c_str(), data.size(), 0, addr, len);
    // }
    // static std::string data(const UDPMessage& data) { return boost::get<0>(data); }
};

template<>
struct SocketHelper<TCPSocket> {
    enum { type = SOCK_STREAM };
    static int touch(int fd) { return accept(fd, 0, 0); }
    // static int accept(int fd) { return ::accept(fd, 0, 0); }
    // static int connect(int fd, const struct sockaddr* serv, socklen_t size) {
    //     return ::connect(fd, serv, size);
    // }
    // static int send(int fd,
    //                 const struct sockaddr*, socklen_t, 
    //                 const std::string& data) {
    //     return ::send(fd, data.c_str(), data.size(), 0);
    // }
    // static std::string data(const std::string& data) { return data; }
};

BOOST_AUTO_TEST_CASE_TEMPLATE(test_socket_init, socket, Sockets) {
    socket s("*", 10000);
    int res = SocketHelper<socket>::touch(s.get());
    BOOST_CHECK(res == -1 && (errno == EWOULDBLOCK || errno == EAGAIN));
}

BOOST_AUTO_TEST_CASE(test_socket_accept) {
    TCPSocket ss("*", 10000);
    BOOST_CHECK_THROW(ss.accept(), std::runtime_error);

    FD cs(socket(AF_INET, SOCK_STREAM, 0));

    struct sockaddr_in server;
    socklen_t size = sizeof(server);
    BOOST_REQUIRE(getsockname(ss.get(), (struct sockaddr*)&server, &size) != -1);
    BOOST_REQUIRE_EQUAL(size, sizeof(server));

    int cnt = connect(cs.get(), (const struct sockaddr*)&server, size);
    BOOST_REQUIRE(cnt != -1 ||
                  (cnt == -1 &&
                   (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)));
    TCPSocket ps(ss.accept());
    BOOST_CHECK(ps.get() != -1);

    struct sockaddr_in client, peer;
    BOOST_REQUIRE(getsockname(cs.get(), (struct sockaddr*)&client, &size) != -1);
    BOOST_REQUIRE_EQUAL(size, sizeof(client));
    BOOST_REQUIRE(getpeername(ps.get(), (struct sockaddr*)&peer, &size) != -1);
    BOOST_REQUIRE_EQUAL(size, sizeof(peer));
    BOOST_CHECK(memcmp(&peer, &client, size) == 0);
    
    int res = ::recv(ps.get(), 0, 0, 0);
    BOOST_CHECK(res != -1 || (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)));
}

BOOST_AUTO_TEST_CASE(test_udp_read) {
    UDPSocket ss("*", 10000);
    struct sockaddr_in server;
    socklen_t size = sizeof(server);
    BOOST_REQUIRE(getsockname(ss.get(), (struct sockaddr*)&server, &size) != -1);
    BOOST_REQUIRE_EQUAL(size, sizeof(server));

    FD cs(::socket(AF_INET, SOCK_DGRAM, 0));
    BOOST_REQUIRE(sendto(cs.get(), "hello", 5, 0,
                         (sockaddr*)&server, sizeof(sockaddr)) != -1);
    char buf[32];
    std::string data(buf, ss.read(buf, sizeof(buf)).first);
    BOOST_CHECK_EQUAL(data, "hello");
}

BOOST_AUTO_TEST_CASE(test_tcp_read) {
    TCPSocket ss("*", 10000);
    struct sockaddr_in server;
    socklen_t size = sizeof(server);
    BOOST_REQUIRE(getsockname(ss.get(), (struct sockaddr*)&server, &size) != -1);
    BOOST_REQUIRE_EQUAL(size, sizeof(server));

    FD cs(::socket(AF_INET, SOCK_STREAM, 0));
    connect(cs.get(), (sockaddr*)&server, sizeof(sockaddr));
    TCPSocket ps = ss.accept();
    BOOST_REQUIRE(send(cs.get(), "hello", 5, 0) != -1);
    char buf[32];
    std::string data(buf, ps.read(buf, sizeof(buf)).first);
    BOOST_CHECK_EQUAL(data, "hello");
}

BOOST_AUTO_TEST_CASE(test_udp_write) {
    UDPSocket ss("*", 10000);
    struct sockaddr_in server;
    socklen_t size = sizeof(server);
    BOOST_REQUIRE(getsockname(ss.get(), (struct sockaddr*)&server, &size) != -1);
    BOOST_REQUIRE_EQUAL(size, sizeof(server));

    FD cs(::socket(AF_INET, SOCK_DGRAM, 0));
    BOOST_REQUIRE(sendto(cs.get(), "hello", 5, 0,
                         (struct sockaddr*)&server, sizeof(sockaddr)) != -1); 
    char buf[32];
    Socket::Request req = ss.read(buf, sizeof(buf));
    ss.write("bye", 3, req.second);
    
    int read = recv(cs.get(), buf, sizeof(buf), 0);
    BOOST_CHECK_EQUAL(read, 3);
    BOOST_CHECK(memcmp("bye", buf, read) == 0);
}

BOOST_AUTO_TEST_CASE(test_tcp_write) {
    TCPSocket ss("*", 10000);
    struct sockaddr_in server;
    socklen_t size = sizeof(server);
    BOOST_REQUIRE(getsockname(ss.get(), (struct sockaddr*)&server, &size) != -1);
    BOOST_REQUIRE_EQUAL(size, sizeof(server));

    FD cs(::socket(AF_INET, SOCK_STREAM, 0));
    connect(cs.get(), (sockaddr*)&server, sizeof(sockaddr));

    TCPSocket ps = ss.accept();
    ps.write("bye", 3);

    char buf[32];
    int read = recv(cs.get(), buf, sizeof(buf), 0);
    BOOST_CHECK_EQUAL(read, 3);
    BOOST_CHECK(memcmp("bye", buf, read) == 0);
}
