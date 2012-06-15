#include "server.h"
#include "socket.h"
#include "pool.h"
#include "client.h"
#include "file_io.h"
#include "sleep.h"
#include "interface.h"
#include "log.h"
#include "socket.h"
#include "poll.h"
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <signal.h>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace {

class ReConfigure : boost::noncopyable {
public:
    ReConfigure(Pipe* notify) : notify_(notify) {
        sigemptyset(&signals_);
        sigaddset(&signals_, SIGUSR1);
        sigaddset(&signals_, SIGHUP);
        CHECK_CALL(pthread_sigmask(SIG_BLOCK, &signals_, NULL), "pthread_sigmask");
    }

    void wait() {
        do {
            int sig;
            CHECK_CALL(sigwait(&signals_, &sig), "sigwait");
            if(sig == SIGUSR1 || sig == SIGHUP) notify(notify_.get());
            else break;
        } while(true);
    }

private:
    sigset_t signals_;
    boost::scoped_ptr<Pipe> notify_;
};

}

Config::Config(const std::string& path) : path_(path) {
    reconfigure();

    REQUIRE(geteuid() == 0 || option<int>("tcp_port") > 1024,
            "Only root can use tcp_port less then 1024");
    REQUIRE(geteuid() == 0 || option<int>("udp_port") > 1024,
            "Only root can use udp_port less then 1024");
}

void Config::reconfigure() {
    po::options_description configuration("Configuration");
    configuration.add_options()
        ("daemon", po::value<bool>()->default_value(false), "Daemon mode")
        ("tcp_if", po::value<std::string>()->default_value("lo"), "TCP interface")
        ("tcp_port", po::value<int>()->default_value(10000), "TCP port")
        ("udp_if", po::value<std::string>()->default_value("lo"), "UDP interface")
        ("udp_port", po::value<int>()->default_value(10000), "UDP port")
        ("datafile", po::value<std::string>(), "Datafile")
        ("sleep", po::value<int>()->default_value(0), "Response timeout")
        ("maint", po::value<bool>()->default_value(false), "Maintenance mode")
        ("loglevel", po::value<std::string>()->default_value("WARN"), "Log level");

    DEBUG("Reading config: " << path_);
    REQUIRE(fs::exists(path_), "Configuration file: " << path_ << " not found"); 
    po::variables_map vm;
    po::store(po::parse_config_file<char>(path_.c_str(), configuration), vm);
    po::notify(vm);

    REQUIRE(vm.count("datafile"), "datafile parameter not specified");
    REQUIRE(vm["tcp_port"].as<int>() > 0 && vm["tcp_port"].as<int>() < 65536,
            "TCP port should be in range 1..65535");
    REQUIRE(vm["udp_port"].as<int>() > 0 && vm["udp_port"].as<int>() < 65536,
            "UDP port should be in range 1..65535");

    config_vm_.swap(vm);
    DEBUG("Configured");
    set_level(option<std::string>("loglevel"));
}



Server::Server(const std::string& path) : config_(path)
{
    Interface iface;
    tcp_.reset(new TCPSocket(iface.host(config_.option<std::string>("tcp_if")),
                             config_.option<int>("tcp_port")));
    udp_.reset(new UDPSocket(iface.host(config_.option<std::string>("udp_if")),
                             config_.option<int>("udp_port")));
}

Server::~Server() {}

void Server::process() {
    Pipe* pipe = new Pipe;
    ReConfigure reconfigure(pipe);
    boost::thread signals(boost::bind(&ReConfigure::wait, &reconfigure));

    Poll poll;

    Sleeper sleeper(1);
    Writer writer(1);
    Pool<TCPClient*, ClientHandler<TCPClient*> > tcp_pool;
    Pool<UDPClient*, ClientHandler<UDPClient*> > udp_pool;

    Message<Pipe, Suppress>* reconfig = new Message<Pipe, Suppress>(pipe);
    poll.add(reconfig);
    TCPClient::Accept* accept = new TCPClient::Accept(tcp_.get());
    poll.add(accept);
    UDPClient::Receive* udp_receive = new UDPClient::Receive(udp_.get());
    poll.add(udp_receive);

    std::string datafile = config_.option<std::string>("datafile");
    int timeout = config_.option<int>("sleep");

    do {
        try {
            poll.perform();
            BOOST_FOREACH(MessageBase* msg, poll) {
                if(msg == udp_receive) {
                    udp_pool.add_task(
                        new UDPClient(udp_receive, datafile, timeout, &sleeper, &writer));
                } else if(msg == accept) {
                    tcp_pool.add_task(
                        new TCPClient(accept, datafile, timeout, &sleeper, &writer));   
                } else if(msg == reconfig) {
                    try {
                        config_.reconfigure();
                        datafile = config_.option<std::string>("datafile");
                        timeout = config_.option<int>("sleep");
                    } catch(std::exception& ex) {
                        ERROR("Reconfiguration failed: " << ex.what());
                    }
                } else {
                    ERROR("Unexpected message: fd(" << msg->fd() << ")");
                    poll.remove(msg);
                }
            }
        } catch(std::runtime_error& ex) {
            ERROR("Unexpected error: " << ex.what());
        }
    } while(true);
    signals.join();
}
