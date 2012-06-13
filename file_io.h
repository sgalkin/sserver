#ifndef SSERVER_FILE_IO_H_INCLUDED
#define SSERVER_FILE_IO_H_INCLUDED

#include "file.h"
#include "message.h"
#include "poll.h"
#include <string>
#include <list>

class Pipe;

struct WriteTask {
    WriteTask(const std::string& filename, const Record& record, Pipe* notify) :
        filename(filename), record(record), notify(notify) {}

    std::string filename;       // one thread for all files not good, but enough here
    Record record;
    Pipe* notify;
};

class Writer : public PollHandler<WriteTask> {
public:
    explicit Writer(Pipe* notify) : PollHandler<WriteTask>(notify) {}

    void add_task(WriteTask client) { clients_.push_back(client); }

private:
    void fetch_message();
    void process_message(MessageBase* msg);

    std::list<WriteTask> clients_;
};

template<typename FDType> class Find;

template<>
class Find<File>: public Input, public Control<File> {
public:
    typedef boost::optional<std::string> type;

    explicit Find(const std::string& name) : name_(name) {}

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
            data_ = find(buf);
            if(data_) return true;
        } while(read == sizeof(buf) - 1);
        return file->seek(0) == file->size();
    }
    
    type data() { return data_; }

private:
    type find(char* buf) {
        char* tail = 0;
        for(char* tok = strtok_r(buf, "\n", &tail);
            tok != 0;
            tok = strtok_r(0, "\n", &tail)) {
            Record rec(record_ + std::string(tok));
            record_.clear();
            if(rec.name() == name_) {
                return rec.email();
            }
        }
        return boost::none;
    }

    std::string name_;
    type data_;
    std::string record_;
};

#endif // SSERVER_FILE_IO_H_INCLUDED
