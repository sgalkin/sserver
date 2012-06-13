#ifndef SSERVER_CLIENT_H_INCLUDED
#define SSERVER_CLIENT_H_INCLUDED

#include "storage.h"
#include "message.h"
#include "sleep.h"
#include "poll.h"
#include "file.h"
#include "file_io.h"

template<typename SocketType>
struct ClientInit;

template<typename SocketType>
class Client {
public:
    enum State { NEW, REQUEST, PENALTY, PROCESS, RESPONSE, DONE };

    typedef Message<TCPSocket, Accept> Accept;
    typedef Message<typename Storage<SocketType>::base, Receive, NOP, Client> Receive;
    typedef Message<Own<Pipe>, Penalty, NOP, Client> Penalty;
    typedef Message<Own<File>, Find, NOP, Client> Find;
    typedef Message<typename Storage<SocketType>::base, NOP, Write, Client> Echo;

    template<typename Parent>
    Client(Parent* parent, const std::string& file, int timeout);

    void notify(Accept* /*msg*/) { forward(REQUEST); }
    void notify(Receive* msg) {
        request_ = msg->data().first;
        target_ = msg->data().second;
        forward(msg->is_eof() ? DONE : PENALTY);
    }
    void notify(Penalty* /*msg*/) { forward(PROCESS); }
    void notify(Find* msg) {
        response_ = (msg->data() ?
                     std::string("200 OK email=") + *msg->data() : std::string("404 Not Found"))
            + "\r\n";
        forward(RESPONSE); 
    }
    void notify(Echo* msg) { forward(msg->is_eof() ? DONE : ClientInit<SocketType>::End); }

    MessageBase* message() {
        if(state_ == REQUEST) {
            return new Receive(Storage<SocketType>::get(socket_), this);
        } else if(state_ == PENALTY) {
            std::auto_ptr<Pipe> pipe(new Pipe);
            sleeper_.add_task(Sleep::Client(pipe.get(), timeout_)); // TODO: notify client not pipe!
            return new Penalty(pipe.release(), this);
        } else if(state_ == PROCESS) {            
            std::auto_ptr<File> file(new File(file_));
            return new Find(file.release(), ::Find<File>(request_), this);
        } else if(state_ == RESPONSE) {
            return new Echo(Storage<SocketType>::get(socket_),
                            Write<typename Storage<SocketType>::base>(
                                response_,
                                boost::bind(&Storage<SocketType>::base::write, _1, _2, _3,
                                            boost::cref(target_))),
                            this);
        } else {
            return 0;
        }
    }

    State state() const { return state_; }

    int get() const { return notify_.reader().get(); }
    int read(char* buf, size_t size) { return notify_.read(buf, size); }

private:
    void forward(State state) {
        ::notify(notify_); // TODO: notify client holder
        state_ = state;
    }

    Pipe notify_;
    typename Storage<SocketType>::type socket_;
    std::string file_;
    Socket::Target target_;
    std::string request_;
    std::string response_;
    int timeout_;
    State state_;

    static Pool<Sleep::Client, Sleep> sleeper_; // TODO: make it singleton
};

template<typename SocketType>
Pool<Sleep::Client, Sleep> Client<SocketType>::sleeper_(1);

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
inline UDPClient::Client(UDPClient::Receive* parent, const std::string& file, int timeout) :
    socket_(ClientInit<UDPSocket>::get(parent)),
    file_(file),
    timeout_(timeout),
    state_(ClientInit<UDPSocket>::Start) {
    notify(parent);
}

template<> template<>
inline TCPClient::Client(TCPClient::Accept* parent, const std::string& file, int timeout) :
    socket_(ClientInit< Own<TCPSocket> >::get(parent)),
    file_(file),
    timeout_(timeout),
    state_(ClientInit< Own<TCPSocket> >::Start) {
    notify(parent);
}

template<typename T> struct remove_ptr { typedef T type; };
template<typename T> struct remove_ptr<T*> { typedef T type; };

template<typename Client>
class ClientHandler : public PollHandler<Client> {
    typedef Message<Own<typename remove_ptr<Client>::type>,
                    Suppress //,
                    // NOP,
                    // ClientHandler
                    > PollMessage;
public:
    explicit ClientHandler(Pipe* pipe) : PollHandler<Client>(pipe) {}

    void add_task(Client clnt) {
        std::auto_ptr<typename remove_ptr<Client>::type> client(clnt);
        this->poll_.add(new PollMessage(client.release(), this));
    }

    // WARNING don't touch poll_ in notify!
    // void notify(PollMessage* msg) {
    //     DEBUG("!!!!");
    // //     if(msg->get()->state() == remove_ptr<Client>::type::DONE) {
    // //         DEBUG("about to delete: " << msg);
    // //         this->poll_.remove(msg);
    // //     } else {
    // //         this->poll_.add(msg->get()->message());
    // //     }
    // }

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
