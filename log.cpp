#include "log.h"

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
    s << asString(level) << " " << buf << " " 
      << prefix_ << "[" << getpid() << "]: " << message << std::endl;
    if(log_fd_ != -1) {
        ::write(log_fd_, s.str().c_str(), s.str().size());
        ::fsync(log_fd_);
    }
    if(isatty(2) == 1) {
        ::write(2, s.str().c_str(), s.str().size());
        ::fsync(2);
    }
}
