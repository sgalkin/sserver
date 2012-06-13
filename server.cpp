#include "server.h"
#include "interface.h"
#include "log.h"
#include "socket.h"
#include "message.h"
#include "poll.h"
#include "sleep.h"
#include "client.h"
#include <boost/foreach.hpp>

namespace {
    typedef Message<TCPSocket, Accept> TCPAccept; 
//    typedef Message<Own<TCPSocket>, Receive> TCPReceive;
    typedef Message<TCPSocket, Receive> TCPReceive;
    typedef Message<UDPSocket, Receive> UDPReceive;
    typedef Message<TCPSocket, NOP, Echo> TCPEcho;
    typedef Message<UDPSocket, NOP, Echo> UDPEcho;

//    typedef Message< Own<Client>, ClientRead, ClientWrite, ClientHangup, ClientError > ClientMessage;
}

enum State { NEW, REQUEST, PENALTY, ECHO, DONE };

class C {
public:
    C(UDPSocket* socket, const SReq& data, int timeout) :
        socket_(socket), data_(data), timeout_(timeout), state_(NEW) {
        notify(notify_);
//        C::sleeper().add_task(Sleep::Client(noty(), timeout_));
    };

    int get() const { return notify_.reader().get(); }
    Pipe* noty() { return &notify_; }
    std::string error() const { return "ERROR"; }

    State state() const { return state_; }
    void next() { if(state_ != DONE) state_ = State(state_ + 1); }

    UDPSocket* socket() { return socket_; }
    const SReq& data() const { return data_; }
    int timeout() const { return timeout_; }

    static Pool<Sleep::Client, Sleep>& sleeper() { return sleeper_; }

private:
    Pipe notify_;
    UDPSocket* socket_;
    SReq data_;
    int timeout_;
    State state_;

    static Pool<Sleep::Client, Sleep> sleeper_;
};

Pool<Sleep::Client, Sleep> C::sleeper_(1);

class TC {
public:
    TC(TCPSocket* socket, int timeout) :
        socket_(socket), data_("eee", boost::none), timeout_(timeout), state_(NEW) {
        DEBUG("tC: " << socket);
        ::notify(notify_);
    }

    int get() const { return notify_.reader().get(); }
    Pipe* noty() { return &notify_; }
    std::string error() const { return "ERROR"; }

    State state() const { return state_; }
    void next() { if(state_ != ECHO) state_ = State(state_ + 1); else state_ = REQUEST; }

    TCPSocket* socket() { return socket_.get(); }
    const SReq& data() const { return data_; }
    int timeout() const { return timeout_; }

    static Pool<Sleep::Client, Sleep>& sleeper() { return sleeper_; }

private:
    Pipe notify_;
    std::auto_ptr<TCPSocket> socket_;
    SReq data_;
    int timeout_;
    State state_;

    static Pool<Sleep::Client, Sleep> sleeper_;
};

Pool<Sleep::Client, Sleep> TC::sleeper_(1);


template<typename F> struct CR;
template<> struct CR<C> : public Input, public Control<C> {
    typedef MessageBase* type;
    bool perform(C* c) { 
        DEBUG(c->state());
        suppress(*c->noty());
        if(c->state() == NEW) {
            DEBUG("!" << c->state());
            C::sleeper().add_task(Sleep::Client(c->noty(), c->timeout()));
        }
        if(c->state() == PENALTY) {
            DEBUG("@" << c->state());
            msg_.reset(new UDPEcho(c->socket(), c->noty(), c->data()));
        }
        c->next(); if(c->state() == REQUEST) c->next();
        return true; 
    }
    type data() { return msg_.release(); }

    std::auto_ptr<MessageBase> msg_;
};

template<> struct CR<TC> : public Input, public Control<TC> {
    typedef MessageBase* type;
    bool perform(TC* c) { 
        suppress(*c->noty());
        DEBUG("### " << c->state());
        if(c->state() == NEW || c->state() == ECHO) {
            msg_.reset(new TCPReceive(c->socket(), c->noty()));
        }
        if(c->state() == REQUEST) {
            C::sleeper().add_task(Sleep::Client(c->noty(), c->timeout()));
        }
        if(c->state() == PENALTY) {
            msg_.reset(new TCPEcho(c->socket(), c->noty(), c->data()));
        }
        c->next();
        return true; 
    }
    type data() { return msg_.release(); }

    std::auto_ptr<MessageBase> msg_;
};


class ClientHandler : public PollHandler<MessageBase*> {
public:
    ClientHandler(Pipe* pipe) : PollHandler<MessageBase*>(pipe) {}

    void add_task(MessageBase* msg) {
        poll_.add(msg);
    }

private:
    void process_message(MessageBase* msg) {
        if(dynamic_cast<Message<Own<C>, CR>*>(msg)) {
            std::auto_ptr<MessageBase> nm((dynamic_cast<Message<Own<C>, CR>*>(msg))->data());
            if(nm.get()) poll_.add(nm.release());
            if(dynamic_cast<Message<Own<C>, CR>*>(msg)->get()->state() == DONE) poll_.remove(msg);
        } else if(dynamic_cast<Message<Own<TC>, CR>*>(msg)) {
            std::auto_ptr<MessageBase> nm((dynamic_cast<Message<Own<TC>, CR>*>(msg))->data());
            if(nm.get()) poll_.add(nm.release());
            if(dynamic_cast<Message<Own<TC>, CR>*>(msg)->get()->state() == DONE) poll_.remove(msg);
        } else {
            DEBUG(msg->fd());
            poll_.remove(msg);
        }
    }
};




Server::Server(const boost::program_options::variables_map& config) : config_(config) {
    Interface iface;
    tcp_.reset(new TCPSocket(iface.host(config["tcp_if"].as<std::string>()),
                             config["tcp_port"].as<unsigned short>()));
    udp_.reset(new UDPSocket(iface.host(config["udp_if"].as<std::string>()),
                             config["udp_port"].as<unsigned short>()));
}

Server::~Server() {}

void Server::process() {
    Poll poll;

    TCPAccept* accept = new TCPAccept(tcp_.get());
    UDPReceive* udp_receive = new UDPReceive(udp_.get());
    poll.add(accept);
    poll.add(udp_receive);
    do {
        poll.perform();
        BOOST_FOREACH(MessageBase* msg, poll) {
            if(msg == udp_receive) {
                std::auto_ptr<C> c(new C(udp_receive->get(),
                                         udp_receive->data(),
                                         config_["sleep"].as<int>()));
                pool_.add_task(new Message<Own<C>, CR>(c.release()));
            } else if(msg == accept) {
                std::auto_ptr<TC> c(new TC(accept->data(),
                                           config_["sleep"].as<int>()));
                pool_.add_task(new Message<Own<TC>, CR>(c.release()));
            } else {
                ERROR("Unexpected message: fd(" << msg->fd() << ")");
                poll.remove(msg);
            }
        }
    } while(true);
}


            // if(msg->fd() == tcp_->get()) {
            //     poll.add(new TCPReceive(accept->data()));
            // } else if(msg->fd() == udp_->get()) {
//                 Pipe* pipe = new Pipe;
//                 sleeper.add_task(Sleep::Client(pipe, config_["sleep"].as<int>()));
//                 poll.add(new Message<Own<Pipe>, Penalty>(
//                              pipe, Penalty<Pipe>(
//                                  new UDPEcho(udp_receive->get(), udp_receive->data()))));
// //                pool_.add_task(new UDPEcho(udp_receive->get(), udp_receive->data()));
//             } else {
//                 if(msg->is_eof() || msg->is_fail()) {
//                     if(msg->is_fail()) {
// //                        ERROR(msg->get_error());
//                         ERROR("error");
//                     }
//                     poll.remove(msg);
//                 } else {
//                     // TODO: remove ugly dynamic cast
//                     TCPReceive* recv = dynamic_cast<TCPReceive*>(msg);
//                     if(!recv) {
//                         //ERROR("unexpected message");
//                         Message<Own<Pipe>, Penalty>* recv = dynamic_cast<Message<Own<Pipe>, Penalty>*>(msg);
//                         pool_.add_task(recv->data());
//                     } else {
//                         Pipe* pipe = new Pipe;
//                         sleeper.add_task(Sleep::Client(pipe, config_["sleep"].as<int>()));
//                         poll.add(new Message<Own<Pipe>, Penalty>(
//                                      pipe, Penalty<Pipe>(
//                                          new TCPEcho(recv->get(), recv->data()))));
// //                        pool_.add_task(new TCPEcho(recv->get(), recv->data()));
//                     }
//                 }
//             }
//         }
//     } while(true);
// }



template<typename F> struct ClientRead;
template<> struct ClientRead<Client> : public Input {
    typedef void* type;
    bool perform(Client* clnt) { 
        DEBUG("CR");
        return true; 
    }
    bool is_eof() const { return false; }
};

template<typename F> struct ClientWrite;
template<> struct ClientWrite<Client> : public Output {
    typedef void* type;
    explicit ClientWrite(void*) {}
    bool perform(Client*) { 
        DEBUG("CW");
        return false; 
    }
    bool is_eof() const { return false; }
};

template<typename F> struct ClientHangup;
template<> struct ClientHangup<Client> { enum { Event = POLLHUP }; 
    bool perform(Client* clt) {
        DEBUG("CH");
        return false; 
    }
    bool is_eof() const { return false; }
};

template<typename F> struct ClientError;
template<> struct ClientError<Client> { enum { Event = POLLERR }; 
    bool perform(Client*) { return false; }
    bool perform(std::string) { return false; }
    operator std::string() const { return "ERROR"; }
};
