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
    typedef Storage<SocketType> Store;
    typedef typename Store::base Base;
    typedef std::map<std::string, MessageBase* (Client::*)(const Request&)> Handler;

public:
    enum State { NEW, REQUEST, PENALTY, PROCESS, RESPONSE, DONE };

    typedef Message<TCPSocket, Accept> TCPAccept;
    typedef Message<Base, Receive<Base>, NOP<Base>, Client> SReceive;
    typedef Message<Own<Pipe>, Suppress<Pipe>, NOP<Pipe>, Client> Penalty;
    typedef Message<Own<File>, Find, NOP<File>, Client> FFind;
    typedef Message<Own<Pipe>, Status<Pipe>, NOP<Pipe>, Client> Register;
    typedef Message<Base, NOP<Base>, Write<Base>, Client> Echo;

    Client(SReceive* parent, const std::string& file, int timeout,
           Sleeper* sleeper, Writer* writer) :
        socket_(ClientInit<SocketType>::get(parent)), file_(file), timeout_(timeout),
        state_(ClientInit<SocketType>::Start),
        sleeper_(sleeper), writer_(writer)
    {
        notify(parent);
    }

    Client(TCPAccept* parent, const std::string& file, int timeout,
           Sleeper* sleeper, Writer* writer) :
        socket_(ClientInit<SocketType>::get(parent)), file_(file), timeout_(timeout),
        state_(ClientInit<SocketType>::Start),
        sleeper_(sleeper), writer_(writer) 
    {
        notify(parent);
    }

    template<typename MessageType>
    void notify(MessageType* msg) {
        try {
            if(msg->is_fail()) throw Unavailable();
            forward(do_notify(msg));
            return;
        } catch(BaseProtocolError& ex) {
            message_.reset(response(ex.what()));
        } catch(std::runtime_error& ex) {
            ERROR("Error while processing message" << ex.what());
            message_.reset(response(Codes::UNAVAILABLE));
        }
        forward(RESPONSE);
    }
    
    MessageBase* message() { return message_.release(); }
    State state() const { return state_; }
    int get() const { return notify_.reader().get(); }
    int read(char* buf, size_t size) { return notify_.read(buf, size); }

private:
    State do_notify(TCPAccept* /*msg*/) { 
        message_.reset(new SReceive(Store::get(socket_), this));
        return REQUEST;
    }

    State do_notify(SReceive* msg) {
        if(msg->is_eof()) return DONE;
        target_ = msg->data().second;
        request_ = Request(msg->data().first);
        std::auto_ptr<Pipe> pipe(new Pipe);
        sleeper_->add_task(Sleep::Client(pipe.get(), timeout_));
        message_.reset(new Penalty(pipe.release(), this));
        return PENALTY;
    }

    State do_notify(Penalty* /*msg*/) {
        typename Handler::const_iterator handler = handlers_.find(request_->method());
        if(handler == handlers_.end() || !request_) {
            message_.reset(response(Codes::BAD_REQUEST));
            return RESPONSE;
        } else {
            message_.reset((this->*(handler->second))(*request_));
            return PROCESS;
        }
    }

    State do_notify(FFind* msg) {
        if(!msg->data().second) {
            message_.reset(response(Codes::NOT_FOUND));
        } else {
            message_.reset(response(Codes::string(Codes::OK) + " email=" + *msg->data().second));
        }
        return RESPONSE;
    }

    State do_notify(Register* msg) {
        message_.reset(response(msg->data()));
        return RESPONSE;
    }
    
    State do_notify(Echo* msg) {
        if(ClientInit<SocketType>::End != DONE)
            message_.reset(new SReceive(Store::get(socket_), this));
        return ClientInit<SocketType>::End;
    }


    MessageBase* get_request(const Request& req) {
        // 1 fd
        std::auto_ptr<File> file(new File(file_));
        return new FFind(file.release(), Find(req.query("username")), this);
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
        return new Echo(Store::get(socket_),
                        Write<Base>(
                            resp + "\r\n",
                            boost::bind(&Base::write, _1, _2, _3, boost::cref(target_))),
                        this);
    }

    MessageBase* response(Code code) {
        return response(Codes::string(code));
    }

    void forward(State state) {
        state_ = state;
        if(state_ == DONE) message_.reset();
        ::notify(notify_);
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

template<typename SocketType>
const typename Client<SocketType>::Handler Client<SocketType>::handlers_ =
    boost::assign::map_list_of
    ("GET", &Client::get_request)
    ("REGISTER", &Client::reg_request).to_container(handlers_);

template<>
struct ClientInit<UDPSocket> {
    typedef UDPSocket type;
    typedef Client<type>::SReceive Parent;
    static const Client<type>::State Start = Client<type>::REQUEST;
    static const Client<type>::State End = Client<type>::DONE;
    static Storage<type>::base* get(Parent* parent) { return parent->get(); }
};

template<>
struct ClientInit< Own<TCPSocket> > {
    typedef Own<TCPSocket> type;
    typedef Client<type>::TCPAccept Parent;
    static const Client<type>::State Start = Client<type>::NEW;
    static const Client<type>::State End = Client<type>::REQUEST;
    static Storage<type>::base* get(Parent* parent) { return parent->data(); }
};

typedef Client<UDPSocket> UDPClient;
typedef Client< Own<TCPSocket> > TCPClient;



template<typename T> struct remove_ptr { typedef T type; };
template<typename T> struct remove_ptr<T*> { typedef T type; };

template<typename Client>
class ClientHandler : public PollHandler<Client> {
    typedef typename remove_ptr<Client>::type ClientType;
    typedef Message<Own<ClientType>, Suppress<ClientType> > PollMessage;
public:
    explicit ClientHandler(Pipe* pipe) : PollHandler<Client>(pipe) {}

    void add_task(Client clnt) {
        std::auto_ptr<ClientType> client(clnt);
        this->poll_.add(new PollMessage(client.release(), this));
    }

    // WARNING don't touch poll_ in notify!

private:
    void process_message(MessageBase* msg) {
        // TODO: remove ugly dynamic_cast
        PollMessage* clnt = dynamic_cast<PollMessage*>(msg);
        if(clnt) {
            if(clnt->get()->state() == ClientType::DONE) {
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
