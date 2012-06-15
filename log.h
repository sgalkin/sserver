#ifndef SSERVER_LOG_H_INCLUDED
#define SSERVER_LOG_H_INCLUDED

#include "fd.h"
#include <string>
#include <boost/utility.hpp>
#include <sstream>

class Logger : boost::noncopyable {
public:
    enum Level { DEBUG = 0, WARN, ERROR, FATAL };

    void write(Level level, const std::string& message);
    void set_level(Level level) { level_ = level; }
    Level level() const { return level_; }

    static Logger& instance();

private:
    explicit Logger(const std::string& exe);

    std::string prefix_;
    Level level_;
    FD log_;
};


inline void set_level(Logger::Level level) {
    Logger::instance().set_level(level);
}

void set_level(const std::string& level);

#define MESSAGE(lvl, message) do {                                      \
        if(Logger::instance().level() <= (lvl)) {                       \
            std::stringstream s_MESSAGE;                                \
            s_MESSAGE << message;                                       \
            Logger::instance().write((lvl), s_MESSAGE.str());           \
        }                                                               \
    } while(false)                                                      \

#define DEBUG(message) MESSAGE(Logger::DEBUG, message)
#define WARN(message) MESSAGE(Logger::WARN, message)
#define ERROR(message) MESSAGE(Logger::ERROR, message)
#define FATAL(message) MESSAGE(Logger::FATAL, message)

#endif // SSERVER_LOG_H_INCLUDED
