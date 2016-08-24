#ifndef LIBLOOM_DUMMYWORKER_H
#define LIBLOOM_DUMMYWORKER_H

#include <string>
#include <uv.h>
#include <memory>
#include <vector>

#include <libloom/connection.h>
#include <libloom/types.h>

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
        return listen_port;
    }

protected:    
    Server &server;

    std::vector<std::unique_ptr<DWConnection>> connections;

    uv_tcp_t listen_socket;
    int listen_port;

    static void _on_new_connection(uv_stream_t *stream, int status);
};

class DWConnection : public loom::SimpleConnectionCallback
{
public:
    DWConnection(DummyWorker &worker);
    ~DWConnection();

    std::string get_peername() {
        return connection.get_peername();
    }

    void accept(uv_tcp_t *listen_socket) {
        connection.accept(listen_socket);
    }

protected:

    void on_message(const char *buffer, size_t size);
    void on_data_chunk(const char *buffer, size_t size);
    void on_data_finish();
    void on_close();

    DummyWorker &worker;
    std::unique_ptr<loom::SendBuffer> send_buffer;
    char *pointer;
    bool registered;
};


#endif // LIBLOOM_DUMMYWORKER_H
