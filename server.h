#ifndef SSERVER_SERVER_H_INCLUDED
#define SSERVER_SERVER_H_INCLUDED

#include <boost/scoped_ptr.hpp>
#include <boost/program_options.hpp>
#include <string>

class TCPSocket;
class UDPSocket;

class Config {
public:
    explicit Config(const std::string& path);

    void reconfigure();

    template<typename T>
    const T& option(const std::string& name) const {
        return config_vm_[name].as<T>();
    }

private:
    std::string path_;
    boost::program_options::variables_map config_vm_;
};


class Server {
public:
    explicit Server(const std::string& path);
    ~Server();

    void process();
    bool is_daemon() const {
        return config_.option<bool>("daemon");
    }

private:
    Config config_;    
    boost::scoped_ptr<TCPSocket> tcp_;
    boost::scoped_ptr<UDPSocket> udp_;
};

#endif // SSERVER_SERVER_H_INCLUDED
