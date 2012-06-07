#ifndef SSERVER_LOG_H_INCLUDED
#define SSERVER_LOG_H_INCLUDED

#include "fd.h"
#include "exception.h"
#include <string>
#include <sstream>
#include <boost/utility.hpp>
#include <limits.h>
#include <stdio.h>

inline std::string exe_path() {
    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "/proc/%d/cmdline", getpid());
    FD(open(buf, O_RDONLY)).read(buf, sizeof(buf));
    std::string path(buf);
    return path.substr(path.rfind('/') + 1);
}

class Logger : boost::noncopyable {
public:
    enum Level { DEBUG = 0, WARN, ERROR, FATAL };

    void write(Level level, const std::string& message);
    void set_level(Level level) { level_ = level; }
    void set_log(int fd) { log_.reset(fd); }
    Level level() const { return level_; }

    static Logger& instance() {
        static Logger logger_(exe_path());
        return logger_;
    }

private:
    explicit Logger(const std::string& prefix) :
        prefix_(prefix), level_(ERROR) {
    }

    std::string prefix_;
    Level level_;
    FD log_;
};


inline void set_level(Logger::Level level) {
    Logger::instance().set_level(level);
}

inline void set_log(int log_fd) {
    Logger::instance().set_log(log_fd);
}

inline const char* asString(Logger::Level level) {
    static const char* strings[] = { "DEBUG", "WARN", "ERROR", "FATAL"};
    REQUIRE(unsigned(level) < sizeof(strings) / sizeof(strings[0]), "Unknown level");
    return strings[level];
}

#define MESSAGE(lvl, message)                           \
    do {                                                \
        if(Logger::instance().level() <= (lvl)) {       \
            std::stringstream s;                        \
            s << message;                               \
            Logger::instance().write((lvl), s.str());   \
        }                                               \
    } while(false)                                      \

#define DEBUG(message) MESSAGE(Logger::DEBUG, message)
#define WARN(message) MESSAGE(Logger::WARN, message)
#define ERROR(message) MESSAGE(Logger::ERROR, message)
#define FATAL(message) MESSAGE(Logger::FATAL, message)

#endif // SSERVER_LOG_H_INCLUDED
