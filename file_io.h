#ifndef SSERVER_FILE_IO_H_INCLUDED
#define SSERVER_FILE_IO_H_INCLUDED

#include "file.h"
#include "message.h"
#include "poll.h"
#include "pool.h"
#include <string>
#include <list>

class Find : public Input, public Control<File> {
public:
    typedef std::pair< int, boost::optional<std::string> > type;

    explicit Find(const std::string& name) :
        name_(name), data_(0, boost::none) {}

    bool perform(File* file);    
    type data() { return data_; }

private:
    boost::optional<std::string> find(char* buf);

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
        return true;
    }
    type data() { return code_; }
private:
    Code code_;
};


class WriteTask {
    typedef Message<Own<File>, Find, NOP<File>, WriteTask> FindRecord;
    typedef Message<Own<File>, NOP<File>, Write<File>, WriteTask> WriteRecord;

    enum State { READ, WRITE, DONE };
public:
    WriteTask(const std::string& filename, const Record& record, Pipe* notify);

    void notify(FindRecord* msg);
    void notify(WriteRecord* msg);

    MessageBase* message();

private:
    std::string filename_; // one thread for all file names not good, but enough here
    Record record_;
    Pipe* notify_;
    State state_;
};

class WriteHandler : public PollHandler<WriteTask> {
public:
    explicit WriteHandler(Pipe* notify) :
        PollHandler<WriteTask>(notify) 
    {}

    void add_task(WriteTask client) { clients_.push_back(client); }

private:
    void add_front();
    void fetch_message();
    void process_message(MessageBase* msg);

    std::list<WriteTask> clients_;
};

typedef Pool<WriteTask, WriteHandler> Writer;

#endif // SSERVER_FILE_IO_H_INCLUDED
