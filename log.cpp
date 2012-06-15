#include "log.h"
#include "exception.h"
#include <limits.h>
#include <stdio.h>
#include <iostream>
#include <boost/thread.hpp>

namespace {

inline std::string exe_path() {
    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "/proc/%d/cmdline", getpid());
    FD(open(buf, O_RDONLY)).read(buf, sizeof(buf));
    return buf;
}

inline std::string cwd() {
    char buf[PATH_MAX];
    REQUIRE(getcwd(buf, sizeof(buf)) != 0, "couldn't get cwd");
    return buf;
}

static struct InitLogger {
    InitLogger() { Logger::instance(); }
} dummy;

static const char* strings[] = { "DEBUG", "WARN", "ERROR", "FATAL"};

inline const char* asString(Logger::Level level) {
    REQUIRE(unsigned(level) < sizeof(strings) / sizeof(strings[0]), "Unknown level");
    return strings[level];
}

inline Logger::Level asLevel(const std::string& name) {
    for(size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); ++i) {
        if(strings[i] == name) return Logger::Level(i);
    }
    THROW("Unknown name: " << name);
}

}

void set_level(const std::string& name) {
    try {
        set_level(asLevel(name));
    } catch(std::runtime_error& ex) {
        ERROR("Couldn't set log level: " << ex.what());
    }
}

Logger& Logger::instance() {
    static Logger logger_(exe_path());
    return logger_;
}

Logger::Logger(const std::string& exe) :
    prefix_(exe.substr(exe.rfind('/') + 1)), level_(ERROR) {
    try {
        log_.reset(open(std::string(cwd() + "/" + prefix_ + ".log").c_str(),
                        O_RDWR | O_APPEND | O_CREAT,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    } catch(std::runtime_error& ex) {
        std::cerr << "ERROR: couldn't open log" << ex.what() << std::endl;
    }
}

void Logger::write(Level level, const std::string& message) {
    if(level < level_) return;
    char buf[20];
    time_t t = time(0);
    struct tm tm;
    strftime(&buf[0], sizeof(buf), "%F %T", localtime_r(&t, &tm));

    std::stringstream s;
    s << asString(level) << " " << buf << " " << prefix_
      << "[" << getpid() << ":" << boost::this_thread::get_id() << "]: "
      << message << std::endl;

    if(log_.get() != -1) {
        while(log_.write(s.str().c_str(), s.str().size()) == -1);
    }
    if(isatty(2) == 1) { 
        ::write(2, s.str().c_str(), s.str().size()); 
    }
}
