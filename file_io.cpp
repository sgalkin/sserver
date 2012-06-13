#include "file_io.h"
#include "code.h"

namespace {
    typedef Message<Own<File>, Find> FindMessage;
    typedef Message<Own<File>, NOP, Write> WriteMessage;
}

void Writer::fetch_message() {
    while(poll_.size() == 1 && !clients_.empty()) {
        const WriteTask& task = clients_.front();
        try {
            std::auto_ptr<File> file(new File(task.filename));
            poll_.add(new FindMessage(file.release(), Find<File>(task.record.name())));
        } catch(std::runtime_error& ex) {
            ERROR("Error while writing data: " << ex.what());
            notify(task.notify, UNAVAILABLE);
            clients_.pop_front();
        }
    }
}

void Writer::process_message(MessageBase* msg) {
    FindMessage* find = dynamic_cast<FindMessage*>(msg);
    if(find) {
        const WriteTask& task = clients_.front();
        if(find->data()) {
            ERROR("Record for: " << task.record.name() << " exists");
            notify(task.notify, CONFLICT);
        } else {
            std::auto_ptr<File> file(new File(task.filename, O_WRONLY | O_APPEND));
            poll_.add(new WriteMessage(file.release(), 
                                       Write<File>(task.record.raw()), task.notify));
        }
        clients_.pop_front();
    }
    if(msg->is_fail()) {
        ERROR("Error while saving data");
    }
    poll_.remove(msg);
}
