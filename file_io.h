#ifndef SSERVER_FILE_IO_H_INCLUDED
#define SSERVER_FILE_IO_H_INCLUDED

#include "file.h"
#include "message.h"
#include "poll.h"
#include "pool.h"
#include <string>
#include <list>

template<typename FDType> class Find;

template<>
class Find<File>: public Input, public Control<File> {
public: 
    typedef std::pair< int, boost::optional<std::string> > type;

    explicit Find(const std::string& name) :
        name_(name), data_(0, boost::none) {}

    bool perform(File* file) {
        char buf[65536];
        int read = 0;
        do {
            read = file->read(buf, sizeof(buf) - 1);
            if(read == 0) return hangup(file);

            buf[read] = 0;
            char* tail = strrchr(buf, '\n');
            if(tail == 0) { // inside very long message
                record_ += std::string(buf);
                continue;
            }
            file->seek((tail - (buf + read)) + 1);
            *tail = 0;
            if((data_.second = find(buf))) return true;
        } while(read == sizeof(buf) - 1);
        return file->seek(0) == file->size();
    }
    
    type data() { return data_; }

private:
    boost::optional<std::string> find(char* buf) {
        char* tail = 0;
        for(char* tok = strtok_r(buf, "\n", &tail);
            tok != 0;
            tok = strtok_r(0, "\n", &tail)) {
            ++data_.first;
            Record rec(record_ + std::string(tok));
            record_.clear();
            if(rec.name() == name_) return rec.email();
        }
        return boost::none;
    }

    std::string name_;
    type data_;
    std::string record_;
};

template<typename FDType>
class Status : public Input, public Control<FDType> {
public:
    typedef Code type;
    Status() : code_(Codes::OK) {}

    bool perform(FDType* fd) {
        code_ = suppress(fd);
        if(code_ != Codes::OK) this->fail();
        return true;
    }
    type data() { return code_; }
private:
    Code code_;
};


class WriteTask {
    typedef Message<Own<File>, Find, NOP, WriteTask> FindRecord;
    typedef Message<Own<File>, NOP, Write, WriteTask> WriteRecord;

    enum State { READ, WRITE, DONE };
public:
    WriteTask(const std::string& filename, const Record& record, Pipe* notify);

    void notify(FindRecord* msg);
    void notify(WriteRecord* msg);

    MessageBase* message();

private:
    std::string filename_;       // one thread for all files not good, but enough here
    Record record_;
    Pipe* notify_;
    State state_;
};

class WriteHandler : public PollHandler<WriteTask> {
public:
    explicit WriteHandler(Pipe* notify) : PollHandler<WriteTask>(notify) {}

    void add_task(WriteTask client) { clients_.push_back(client); }

private:
    void add_front();
    void fetch_message();
    void process_message(MessageBase* msg);

    std::list<WriteTask> clients_;
};

typedef Pool<WriteTask, WriteHandler> Writer;

#endif // SSERVER_FILE_IO_H_INCLUDED
