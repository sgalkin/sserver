#ifndef SSERVER_LOG_H_INCLUDED
#define SSERVER_LOG_H_INCLUDED

#include "exception.h"
#include <string>
#include <sstream>
#include <boost/utility.hpp>

#include <iostream>

class Logger : boost::noncopyable {
public:
    enum Level { DEBUG = 0, WARN, ERROR, FATAL };

    ~Logger() { close_log_fd(); }

    void write(Level level, const std::string& message);
    void set_level(Level level) { level_ = level; }
    void set_log(int log_fd) { close_log_fd(); log_fd_ = log_fd; }
    Level level() const { return level_; }

    static Logger& instance() {
        // TODO: detemine process name
        static Logger logger_("aaa");
        return logger_;
    }

private:
    explicit Logger(const std::string& prefix) :
        prefix_(prefix), level_(ERROR), log_fd_(-1) {
    }

    void close_log_fd() {
        if(log_fd_ != -1) {
            close(log_fd_);
            log_fd_ = -1;
        }
    }

    std::string prefix_;
    Level level_;
    int log_fd_;
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
#define WANR(message) MESSAGE(Logger::WARN, message)
#define ERROR(message) MESSAGE(Logger::ERROR, message)
#define FATAL(message) MESSAGE(Logger::FATAL, message)

#endif // SSERVER_LOG_H_INCLUDED
