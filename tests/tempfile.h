#ifndef SSERVER_TEMPFILE_H_INCLUDED
#define SSERVER_TEMPFILE_H_INCLUDED

#include "../fd.h"
#include <cstdlib>
#include <string>

class TempFile {
public:
    explicit TempFile(const std::string& base) :
        path_(base + ".XXXXXX"), fd_(mkstemp(&path_[0]))
    {}

    ~TempFile() {
        unlink(path_.c_str());
    }

    const std::string& name() const { return path_; }
    int write(const char* buf, size_t size) { return fd_.write(buf, size); }
    int read(char* buf, size_t size) { return fd_.read(buf, size); }

private:
    std::string path_;
    FD fd_;
};

#endif // SSERVER_TEMPFILE_H_INCLUDED
