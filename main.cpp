#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <ifaddrs.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>


namespace po = boost::program_options;
namespace fs = boost::filesystem;

#define REQUIRE(cond, message)                  \
    do {                                        \
        if(!(cond)) {                           \
            std::stringstream s;                \
            s << message;                       \
            throw std::runtime_error(s.str());  \
        }                                       \
    } while(false)                              \

#define CHECK_CALL(callee, message)             \
    do {                                        \
        if((callee) == -1) {                    \
            std::stringstream s;                \
            s << (message) << " ";              \
            throw std::runtime_error(s.str());                     \
        }                                                          \
    } while(false)                                                 \

//TODO: don't forget to drop privilegies after bind

class Interfaces : boost::noncopyable {
public:
    Interfaces() {
        CHECK_CALL(getifaddrs(&interfaces_),
                   "Error while reading interfaces: ");
    }

    operator const struct ifaddrs*() const {
        return interfaces_;
    }

    ~Interfaces() {
        freeifaddrs(interfaces_);
    }

private:
    struct ifaddrs* interfaces_;
};


int main(int argc, char* argv[]) {
    po::options_description configuration("Configuration");
//TODO: think about negative port number
    configuration.add_options()
        ("daemon", po::value<bool>()->default_value(false), "Daemon mode")
        ("tcp_if", po::value<std::string>()->default_value("lo"), "TCP interface")
        ("tcp_port", po::value<unsigned short>()->default_value(10000), "TCP port")
        ("udp_if", po::value<std::string>()->default_value("lo"), "UDP interface")
        ("udp_port", po::value<unsigned short>()->default_value(10000), "UDP port")
        ("datafile", po::value<std::string>(), "Datafile")
        ("sleep", po::value<unsigned>()->default_value(0), "Response timeout")
        ("maint", po::value<bool>()->default_value(false), "Maintenance mode")
        ("loglevel", po::value<std::string>()->default_value("WARN"), "Log level");

    po::options_description program("Program options");
    program.add_options()
        ("help,h", "Help")
        ("config,c", po::value<std::string>()->default_value("/etc/sserver.conf"));

    try {
        po::variables_map program_vm;
        po::store(po::parse_command_line(argc, argv, program), program_vm);
        po::notify(program_vm);

        if(program_vm.count("help")) {
            std::cerr << program << "\n" << configuration << std::endl;
            return 0;
        }

        std::string config = program_vm["config"].as<std::string>();
        REQUIRE(fs::exists(config), "Configuration file: " << config << " not found");

        po::variables_map config_vm;
        po::store(po::parse_config_file<char>(config.c_str(), configuration), config_vm);
        po::notify(config_vm);
        REQUIRE(config_vm.count("datafile"), "datafile parameter not specified");

    } catch(std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
    Interfaces i;
    CHECK_CALL(open("foo", O_RDONLY), "error");
    return 0;
}
