#include "file_io.h"
#include "code.h"

namespace { 
const int MAX_RECORDS = 100;
}

WriteTask::WriteTask(const std::string& filename, const Record& record, Pipe* notify) :
    filename_(filename), record_(record), notify_(notify), state_(READ) 
{}

void WriteTask::notify(FindRecord* msg) {
    DEBUG("AAA: " << msg->data().first);
    if(msg->is_fail()) {
        ::notify(notify_, Codes::UNAVAILABLE);
        state_ = DONE;
    } else if(msg->data().second) {
        DEBUG("Record for: " << record_.name() << " exists");
        ::notify(notify_, Codes::CONFLICT);
        state_ = DONE;
    } else if(msg->data().first > MAX_RECORDS) { // 101 is more then 100
        DEBUG("Too many record: " << msg->data().first);
        ::notify(notify_, Codes::OVERLOADED);
        state_ = DONE;
    } else {
        state_ = WRITE;
    }
}

void WriteTask::notify(WriteRecord* msg) {
    if(msg->is_fail()) {
        ::notify(notify_, Codes::UNAVAILABLE);
    } else {
        ::notify(notify_, Codes::OK);
    }
    state_ = DONE;
}

MessageBase* WriteTask::message() {
    try {
        if(state_ == READ) {
            std::auto_ptr<File> file(new File(filename_)); // don't open too many files
            return new FindRecord(file.release(), Find<File>(record_.name()), this);
        } else if(state_ == WRITE) {
            std::auto_ptr<File> file(new File(filename_, O_WRONLY | O_APPEND));
            return new WriteRecord(file.release(), Write<File>(record_.raw()), this);
        }
    } catch(std::runtime_error& ex) {
        ERROR("Error while writing data: " << ex.what()); 
        state_ = DONE;
        ::notify(notify_, Codes::UNAVAILABLE);
    }
    return 0;
}

void WriteHandler::add_front() {
    std::auto_ptr<MessageBase> msg(clients_.front().message());
    if(!msg.get()) clients_.pop_front();
    else poll_.add(msg.release());
}

void WriteHandler::fetch_message() {
    while(poll_.size() == 1 && !clients_.empty()) {
        add_front();
    }
}

void WriteHandler::process_message(MessageBase* msg) {    
    poll_.remove(msg);
    if(!clients_.empty()) {
        add_front();
        // WriteTask& task = clients_.front();
        // if(task.state() == WriteTask::DONE) {
        //     clients_.pop_front();
        // } else {
        //     add_front();
        // }
    }
}
