#include "log.h"
#include <boost/thread.hpp>

namespace {

static struct InitLogger {
    InitLogger() { Logger::instance(); }
} dummy;

}

void Logger::write(Level level, const std::string& message) {
    if(level < level_) return;
    // TODO: set descriptor here
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
    // TODO: fix it
    /*if(isatty(2) == 1)*/ { ::write(2, s.str().c_str(), s.str().size()); }
}
