#ifndef SSERVER_SLEEP_H_INCLUDED
#define SSERVER_SLEEP_H_INCLUDED

#include <list>

class Pipe;

class Sleep {
public:
    typedef std::pair<Pipe*, int> Client;

    Sleep(Pipe* pipe) : pipe_(pipe) {}

    void add_task(Client client);
    void perform();

private:
    typedef std::list<Client> Clients;

    Pipe* pipe_;
    Clients clients_;
};

#endif // SSERVER_SLEEP_H_INCLUDED
