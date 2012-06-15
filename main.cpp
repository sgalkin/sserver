#include "server.h"
#include "exception.h"
#include "log.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <pwd.h>
#include <sys/types.h>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace {

void child() {
    pid_t pid;
    CHECK_CALL((pid = ::fork()), "fork");
    if(pid > 0) _exit(0);
}

void daemonize() {
    DEBUG("Daemonization");
    child();
    CHECK_CALL(setsid(), "setsid");
    umask(0);
    CHECK_CALL(chdir("/"), "chdir");
    child();
    close(0);
    close(1);
    close(2);
    CHECK_CALL(open("/dev/null", O_RDONLY), "stdin");
    CHECK_CALL(open("/dev/null", O_RDWR), "stdout");
    CHECK_CALL(open("/dev/null", O_RDWR), "stderr");
    DEBUG("Daemon is running");
}

void switch_user(const std::string& user) {
    DEBUG("Changing user to: " << user);
    std::string buf(sysconf(_SC_GETPW_R_SIZE_MAX), 0);
    struct passwd pwd;
    struct passwd* result = 0;
    CHECK_CALL(getpwnam_r(user.c_str(), &pwd, &buf[0], buf.size(), &result), "getpwnam_r");
    CHECK_CALL(setuid(pwd.pw_uid), "setuid");
    DEBUG("User changed to: " << user);
}

std::string real_path(const std::string& path) {
    char buf[PATH_MAX];
    REQUIRE(realpath(path.c_str(), buf) != NULL, "realpath");
    return buf;
}

}

int main(int argc, char* argv[]) {
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
            std::cerr << program << std::endl;
            return 0;
        }

        Server server(real_path(program_vm["config"].as<std::string>()));
        if(getuid() == 0) switch_user(program_vm["user"].as<std::string>());
        if(server.is_daemon()) daemonize();
        server.process();

    } catch(std::exception& ex) {
        FATAL(ex.what());
        return 1;
    }
    return 0;
}
