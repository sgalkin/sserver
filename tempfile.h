#ifndef SSERVER_TEMPFILE_H_INCLUDED
#define SSERVER_TEMPFILE_H_INCLUDED

#include "fd.h"
#include <cstdlib>

class TempFile {
public:
    explicit TempFile(const std::string& base) :
        path_(base + ".XXXXXX"), fd_(mkstemp(&path_[0]))
    {}

    ~TempFile() {
        unlink(path_.c_str());
    }

    const std::string& name() const { return path_; }

    // void rename(const std::string& newname) {
    //     CHECK_CALL(::rename(path_.c_str(), newname.c_str()));
    //     path.clear();
    // }

private:
    std::string path_;
    FD fd_;
};

#endif // SSERVER_TEMPFILE_H_INCLUDED
