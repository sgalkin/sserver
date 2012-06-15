#ifndef SSERVER_CLIENT_H_INCLUDED
#define SSERVER_CLIENT_H_INCLUDED

#include "storage.h"
#include "message.h"
#include "sleep.h"
#include "poll.h"
#include "request.h"
#include "file.h"
#include "file_io.h"
#include <boost/assign.hpp>

template<typename SocketType>
struct ClientInit;

template<typename SocketType>
class Client {
    typedef std::map<std::string, MessageBase* (Client::*)(const Request&)> Handler;
public:
    enum State { NEW, REQUEST, PENALTY, PROCESS, RESPONSE, DONE };

    typedef Message<TCPSocket, Accept> Accept;
    typedef Message<typename Storage<SocketType>::base, Receive, NOP, Client> Receive;
    typedef Message<Own<Pipe>, Suppress, NOP, Client> Penalty;
    typedef Message<Own<File>, Find, NOP, Client> Find;
    typedef Message<Own<Pipe>, Status, NOP, Client> Register;
    typedef Message<typename Storage<SocketType>::base, NOP, Write, Client> Echo;

    template<typename Parent>
    Client(Parent* parent, const std::string& file, int timeout,
           Sleeper* sleeper, Writer* writer);

    void notify(Accept* /*msg*/) { 
        message_.reset(new Receive(Storage<SocketType>::get(socket_), this));
        forward(REQUEST);
    }

    void notify(Receive* msg) {
        forward(msg->is_eof() ? DONE : PENALTY);
        if(state_ == DONE) return;
        try {
            target_ = msg->data().second;
            request_ = Request(msg->data().first);
            std::auto_ptr<Pipe> pipe(new Pipe);
            sleeper_->add_task(Sleep::Client(pipe.get(), timeout_)); // TODO: notify client not pipe!
            message_.reset(new Penalty(pipe.release(), this));
        } catch(BaseProtocolError& ex) {
            message_.reset(response(ex.what()));
            forward(RESPONSE);
        }
    }

    void notify(Penalty* /*msg*/) {
        typename Handler::const_iterator handler = handlers_.find(request_->method());
        if(handler == handlers_.end() || !request_) {
            message_.reset(response(Codes::BAD_REQUEST));
            forward(RESPONSE);
        } else {
            try {
                message_.reset((this->*(handler->second))(*request_));
                forward(PROCESS);
            } catch (std::runtime_error& ex) {
                ERROR("Error while processing message" << ex.what());
                message_.reset(response(Codes::UNAVAILABLE));
                forward(RESPONSE);
            }
        }
    }

    void notify(Find* msg) {
        if(msg->is_fail()) {
            message_.reset(response(Codes::UNAVAILABLE));
        } else if(!msg->data().second) {
            message_.reset(response(Codes::NOT_FOUND));
        } else {
            message_.reset(response(Codes::string(Codes::OK) + " email=" + *msg->data().second));
        }
        forward(RESPONSE);
    }

    void notify(Register* msg) {
        message_.reset(response(msg->data()));
        forward(RESPONSE);
    }
    
    void notify(Echo* msg) {
        forward(msg->is_eof() ? DONE : ClientInit<SocketType>::End);
        if(state_ != DONE)
            message_.reset(new Receive(Storage<SocketType>::get(socket_), this));
    }
    
    MessageBase* message() { return message_.release(); }

    State state() const { return state_; }

    int get() const { return notify_.reader().get(); }
    int read(char* buf, size_t size) { return notify_.read(buf, size); }

private:
    MessageBase* get_request(const Request& req) {
        // 1 fd
        std::auto_ptr<File> file(new File(file_));
        return new Find(file.release(), ::Find<File>(req.query("username")), this);
    }

    MessageBase* reg_request(const Request& req) {
        // 3 fd
        std::auto_ptr<Pipe> pipe(new Pipe);
        writer_->add_task(WriteTask(file_,
                                    Record(req.query("username"), req.query("email")),
                                    pipe.get()));
        return new Register(pipe.release(), this);
    }

    MessageBase* response(const std::string& resp) {
        return new Echo(Storage<SocketType>::get(socket_),
                        Write<typename Storage<SocketType>::base>(
                            resp + "\r\n",
                            boost::bind(&Storage<SocketType>::base::write,
                                        _1, _2, _3, boost::cref(target_))),
                        this);
    }

    MessageBase* response(Code code) {
        return response(Codes::string(code));
    }

    void check_message(MessageBase* msg) {
        // if(msg->is_fail()) forward(ClientInit<SocketType>::End);
        // if(msg->is_eof()) forward(DONE);
    }

    void forward(State state) {
        state_ = state;
        if(state_ == DONE) message_.reset();
        ::notify(notify_); // TODO: notify client holder
    }

    Pipe notify_;   // TODO remove it
    typename Storage<SocketType>::type socket_;
    std::string file_;
    Socket::Target target_;
    boost::optional<Request> request_;
    int timeout_;
    State state_;
    std::auto_ptr<MessageBase> message_;

    Sleeper* sleeper_;
    Writer* writer_;
    static const Handler handlers_; // TODO: remove
};

// TODO bad practice
template<typename SocketType>
const typename Client<SocketType>::Handler Client<SocketType>::handlers_ =
    boost::assign::map_list_of
    ("GET", &Client::get_request)
    ("REGISTER", &Client::reg_request).to_container(handlers_);

template<>
struct ClientInit<UDPSocket> {
    typedef Client<UDPSocket>::Receive Parent;
    typedef UDPSocket type;
    static const Client<type>::State Start = Client<type>::REQUEST;
    static const Client<type>::State End = Client<type>::DONE;
    static Storage<type>::base* get(Parent* parent) { return parent->get(); }
};

template<>
struct ClientInit< Own<TCPSocket> > {
    typedef Client< Own<TCPSocket> >::Accept Parent;
    typedef Own<TCPSocket> type;
    static const Client<type>::State Start = Client<type>::NEW;
    static const Client<type>::State End = Client<type>::REQUEST;
    static Storage<type>::base* get(Parent* parent) { return parent->data(); }
};

typedef Client<UDPSocket> UDPClient;
typedef Client< Own<TCPSocket> > TCPClient;

template<> template<>
inline UDPClient::Client(UDPClient::Receive* parent, const std::string& file, int timeout,
                         Sleeper* sleeper, Writer* writer) :
    socket_(ClientInit<UDPSocket>::get(parent)), file_(file), timeout_(timeout),
    state_(ClientInit<UDPSocket>::Start),
    sleeper_(sleeper), writer_(writer) 
{
    notify(parent);
}

template<> template<>
inline TCPClient::Client(TCPClient::Accept* parent, const std::string& file, int timeout,
                         Sleeper* sleeper, Writer* writer) :
    socket_(ClientInit< Own<TCPSocket> >::get(parent)), file_(file), timeout_(timeout),
    state_(ClientInit< Own<TCPSocket> >::Start),
    sleeper_(sleeper), writer_(writer) 
{
    notify(parent);
}

template<typename T> struct remove_ptr { typedef T type; };
template<typename T> struct remove_ptr<T*> { typedef T type; };

template<typename Client>
class ClientHandler : public PollHandler<Client> {
    typedef Message<Own<typename remove_ptr<Client>::type>, Suppress> PollMessage;
public:
    explicit ClientHandler(Pipe* pipe) : PollHandler<Client>(pipe) {}

    void add_task(Client clnt) {
        std::auto_ptr<typename remove_ptr<Client>::type> client(clnt);
        this->poll_.add(new PollMessage(client.release(), this));
    }

    // WARNING don't touch poll_ in notify!

private:
    void process_message(MessageBase* msg) {
        PollMessage* clnt = dynamic_cast<PollMessage*>(msg); // TODO: remove ugly dynamic_cast
        if(clnt) {
            if(clnt->get()->state() == remove_ptr<Client>::type::DONE) {
                this->poll_.remove(msg);
            } else {
                this->poll_.add(clnt->get()->message());
            }            
        } else {
            this->poll_.remove(msg);
        }
    }
};


#endif // SSERVER_CLIENT_H_INCLUDED
