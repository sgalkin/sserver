#include "file_io.h"
#include "code.h"

WriteTask::WriteTask(const std::string& filename, const Record& record, Pipe* notify) :
    filename_(filename), record_(record), notify_(notify), state_(READ) {
}

void WriteTask::notify(FindRecord* msg) {
    if(msg->is_fail()) {
        ::notify(notify_, UNAVAILABLE);
        state_ = DONE;
    } else if(msg->data()) { 
        ERROR("Record for: " << record_.name() << " exists");
        ::notify(notify_, CONFLICT);
        state_ = DONE;
    } else {
        state_ = WRITE;
    }
}

void WriteTask::notify(WriteRecord* msg) {
    if(msg->is_fail()) {
        ::notify(notify_, UNAVAILABLE);
    } else {
        ::notify(notify_);
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
        } else {
            return 0;
        }
    } catch(std::runtime_error& ex) {
        ERROR("Error while writing data: " << ex.what()); 
        state_ = DONE;
        ::notify(notify_, UNAVAILABLE);
        throw;
    }
}

void Writer::add_front() {
    WriteTask& task = clients_.front();
    try {
        poll_.add(task.message());
    } catch(std::runtime_error& ex) {
        clients_.pop_front();
    }
}

void Writer::fetch_message() {
    while(poll_.size() == 1 && !clients_.empty()) {
        add_front();
    }
}

void Writer::process_message(MessageBase* msg) {    
    poll_.remove(msg);
    if(!clients_.empty()) {
        WriteTask& task = clients_.front();
        if(task.state() == WriteTask::DONE) {
            clients_.pop_front();
        } else {
            add_front();
        }
    }
}
