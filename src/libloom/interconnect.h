#ifndef LOOM_INTERCONNECT_H
#define LOOM_INTERCONNECT_H

#include "connection.h"
#include "data.h"

#include <memory>
#include <vector>

namespace loom {

class Worker;
class DataBuilder;
class SendBuffer;

/** Interconnection between workers */
class InterConnection : public SimpleConnectionCallback
{
public:
    InterConnection(Worker &worker);
    ~InterConnection();

    void send(Id id, std::shared_ptr<Data> &data, bool with_size);
    void accept(uv_tcp_t *listen_socket) {
        connection.accept(listen_socket);
    }
    std::string get_peername() {
        return connection.get_peername();
    }
    std::string get_address() const {
        return address;
    }

    void connect(const std::string &address, int port) {
        this->address = make_address(address, port);
        SimpleConnectionCallback::connect(address, port);
    }

protected:

    void _send(SendBuffer &buffer);

    void on_message(const char *buffer, size_t size);
    void on_data_chunk(const char *buffer, size_t size);
    void on_data_finish();
    void on_connection();
    void on_close();

    void finish_data();

    Worker &worker;
    std::string address;

    std::unique_ptr<DataUnpacker> data_unpacker;
    Id data_id;

    static std::string make_address(const std::string &host, int port);

    std::vector<std::unique_ptr<SendBuffer>> early_sends;
};

}

#endif // LOOM_INTERCONNECT_H
