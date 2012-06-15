#ifndef SSERVER_MESSAGE_H_INCLUDED
#define SSERVER_MESSAGE_H_INCLUDED

#include "io.h"
#include "code.h"
#include "log.h"
#include "storage.h"

class Pipe;

class MessageBase {
public:
    virtual ~MessageBase() {}
    
    virtual bool is_ready() const = 0;
    virtual bool is_eof() const = 0;
    virtual bool is_fail() const = 0;

    void read() { do_notify(&MessageBase::do_read); }
    void write() { do_notify(&MessageBase::do_write); }
    void hangup() { do_notify(&MessageBase::do_hangup); }
    void error() { do_notify(&MessageBase::do_error); }

    virtual int fd() const = 0;
    virtual int events() const = 0;
    virtual void notify() = 0;
    
private:
    void do_notify(void (MessageBase::*op)()) {
        (this->*op)();
        notify();
    }

    virtual void do_read() = 0;
    virtual void do_write() = 0;
    virtual void do_hangup() = 0;
    virtual void do_error() = 0;
};


template<typename T>
struct NotifyHelper {
    template<typename U>
    static void notify(T* notify, U* data) { notify->notify(data); }
};

template<>
struct NotifyHelper<Pipe> {
    template<typename U>
    static void notify(Pipe* notify, U*) { ::notify(notify); }
};

template<typename H, typename F>
bool try_do(H& handler, F fd) {
    try {
        return handler.perform(fd);
    } catch(std::runtime_error& ex) {
        ERROR("IO error: " << ex.what());
        handler.fail();
    }
    return true;
}

template<typename Type,
         typename Read = NOP<typename Storage<Type>::base>,
         typename Write = NOP<typename Storage<Type>::base>,
         typename Notify = Pipe>
class Message : public MessageBase {
    typedef Storage<Type> Store;
    typedef typename Store::base FDType;
public:
    enum { Event = Read::Event | Write::Event | POLLHUP | POLLERR };

    explicit Message(FDType* fd, Notify* notify = 0,
                     const typename Write::type& data = typename Write::type()) :
        fd_(fd), notify_(notify), writer_(data), ready_(false) {}

    Message(FDType* fd, const Read& reader, Notify* notify = 0) :
        fd_(fd), notify_(notify), reader_(reader), ready_(false) {}

    Message(FDType* fd, const Write& writer, Notify* notify = 0) :
        fd_(fd), notify_(notify), writer_(writer), ready_(false) {}

    // in C++11 move data from reader
    // getting data from reader may not be constant
    typename Read::type data() { return reader_.data(); }
//    const typename Read<FDType>::type& data() const { return reader_.data(); }

    bool is_ready() const { return ready_ || is_eof() || is_fail(); }
    bool is_eof() const { return reader_.is_eof() || writer_.is_eof(); }
    bool is_fail() const { return reader_.is_fail() || writer_.is_fail(); }

    FDType* get() { return Store::get(fd_); }
    const FDType* get() const { return Store::get(fd_); }
    int fd() const { return GetFD<FDType, Read, Write>::get(this->get()); }
    int events() const { return Event; }

private:
    void notify() { if(notify_) NotifyHelper<Notify>::notify(notify_, this); }

    void do_read() { ready_ = try_do(reader_, Store::get(fd_)); }
    void do_write() { ready_ = try_do(writer_, Store::get(fd_)); }
    void do_hangup() {
        reader_.hangup(Store::get(fd_));
        writer_.hangup(Store::get(fd_));
    }
    void do_error() {
        reader_.fail(Store::get(fd_));
        writer_.fail(Store::get(fd_));
    }

    typename Store::type fd_;
    Notify* notify_;
    Read reader_;
    Write writer_;
    bool ready_;
};


#endif // SSERVER_MESSAGE_H_INCLUDED
