#ifndef SSERVER_FILE_H_INCLUDED
#define SSERVER_FILE_H_INCLUDED

#include "fd.h"
#include "exception.h"
#include <sys/stat.h>
#include <unistd.h>

class File {
public:
    explicit File(const std::string& filename, int flags = O_RDONLY) :
        fd_(open(filename.c_str(), flags))
    {}

    int size() const {
        struct stat stat;
        CHECK_CALL(fstat(fd_.get(), &stat), "fstat");
        return stat.st_size;
    }

    int read(char* buf, size_t size) { return fd_.read(buf, size); }
    int write(const char* buf, size_t size) { return fd_.write(buf, size); }
    off_t seek(off_t offset) { 
        off_t pos;
        CHECK_CALL((pos = lseek(fd_.get(), offset, SEEK_CUR)), "lseek");
        return pos;
    }

    int get() const { return fd_.get(); }

private:
    FD fd_;
};


class Record {
public:
    explicit Record(const std::string& record) :
        name_(record.substr(0, record.find(';'))),
        email_(record.find(';') == std::string::npos ? std::string() :
               record.substr(record.find(';') + 1))
    {}

    Record(const std::string& name, const std::string& email) :
        name_(name), email_(email)
    {}

    const std::string& name() const { return name_; }
    const std::string& email() const { return email_; }
    std::string raw() const { return name_ + ";" + email_ + "\n"; }

private:
    std::string name_;
    std::string email_;
};

#endif // SSERVER_FILE_H_INCLUDED
