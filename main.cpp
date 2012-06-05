#include "server.h"
#include "exception.h"
#include "log.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <pwd.h>
#include <sys/types.h>

#include <fcntl.h>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

void child() {
    pid_t pid;
    if((pid = ::fork()) < 0) {
        char dummy;
        THROW("fork: " << strerror_r(errno, &dummy, sizeof(dummy)));
    }
    else if(pid > 0) exit(0);
}

void daemonize() {
    DEBUG("Daemonization");
    child();
    CHECK_CALL(setsid(), "setsid");
    umask(0);
    CHECK_CALL(chdir("/"), "chdir");
    child();
    // TODO: close all descriptors
    // TODO: change to constants
    close(0);
    close(1);
    close(2);
//    for(int i = 0; i < sysconf(_SC_OPEN_MAX); ++i) close(i);
    CHECK_CALL(open("/dev/null", O_RDONLY), "stdin");
    CHECK_CALL(open("/dev/null", O_RDWR), "stdout");
    CHECK_CALL(open("/dev/null", O_RDWR), "stderr");
}

void switch_user(const std::string& user) {
    DEBUG("Changing user to: " << user);
    std::string buf(sysconf(_SC_GETPW_R_SIZE_MAX), 0);
    struct passwd pwd;
    struct passwd* result = 0;
    CHECK_CALL(getpwnam_r(user.c_str(), &pwd, &buf[0], buf.size(), &result), "getpwnam_r");
    CHECK_CALL(setuid(pwd.pw_uid), "setuid");
    DEBUG("Changed user to: " << user);
}

int main(int argc, char* argv[]) {
    po::options_description configuration("Configuration");
//TODO: think about negative port number
//TODO: think about interface aliases
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
        ("config,c", po::value<std::string>()->default_value("/etc/sserver.conf"))
        ("user,u", po::value<std::string>()->default_value("nobody"),
         "Unprivileged user switch to");

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

        DEBUG("Reading config: " << config);
        po::variables_map config_vm;
        po::store(po::parse_config_file<char>(config.c_str(), configuration), config_vm);
        po::notify(config_vm);
        set_level(Logger::DEBUG);
        // CHECK_CALL(open((std::string(argv[0]) + ".log").c_str(), 
        //                 O_WRONLY | O_CREAT | O_APPEND,
        //                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), "open log failed");
        // set_log(3);
        REQUIRE(config_vm.count("datafile"), "datafile parameter not specified");
        REQUIRE(geteuid() == 0 || config_vm["tcp_port"].as<unsigned short>() > 1024,
                "Only root can use tcp_port less then 1024");
        REQUIRE(geteuid() == 0 || config_vm["udp_port"].as<unsigned short>() > 1024,
                "Only root can use udp_port less then 1024");

        Server server(config_vm);
        // for(int i = 0; i < 1024; ++i) {
        //     if(fcntl(i, F_GETFL) != -1) std::cout << i << std::endl;
        // }
        if(getuid() == 0) {
            switch_user(program_vm["user"].as<std::string>());
        }
        if(config_vm["daemon"].as<bool>()) daemonize();
        // std::ofstream of("/home/sergo/work/skype/sserver/123");
        // std::cout << sysconf(_SC_OPEN_MAX) << std::endl;
        // for(int i = 0; i < 1024; ++i) {
        //     if(fcntl(i, F_GETFL) != -1) of << i << std::endl;
        // }

        server.process();

    } catch(std::exception& ex) {
        FATAL(ex.what());
        return 1;
    }
    return 0;
}
