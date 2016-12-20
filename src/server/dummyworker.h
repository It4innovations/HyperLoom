#ifndef LIBLOOM_DUMMYWORKER_H
#define LIBLOOM_DUMMYWORKER_H

#include <string>
#include <uv.h>
#include <memory>
#include <vector>

#include <libloomnet/listener.h>
#include <libloomnet/socket.h>
#include <libloomnet/sendbuffer.h>
#include <libloomw/types.h>

class Server;
class DWConnection;

/** An implementation of a simple worker that is only able
 *  to receive a data
 */
class DummyWorker
{
    friend class DWConnection;
public:
    DummyWorker(Server &server);

    void start_listen();

    std::string get_address() const;
    int get_listen_port() const {
        return listener.get_port();
    }

    Server& get_server() {
        return server;
    }

protected:    
    Server &server;

    std::vector<std::unique_ptr<DWConnection>> connections;

    loom::net::Listener listener;
};

class DWConnection
{
public:
    DWConnection(DummyWorker &worker);
    ~DWConnection();

    std::string get_peername() {
        return socket.get_peername();
    }

    void accept(loom::net::Listener &listener);


protected:

    void on_message(const char *buffer, size_t size);

    DummyWorker &worker;
    loom::net::Socket socket;
    std::unique_ptr<loom::net::SendBuffer> send_buffer;
    size_t remaining_messages;
    bool registered;
};


#endif // LIBLOOM_DUMMYWORKER_H
