#ifndef LOOM_INTERCONNECT_H
#define LOOM_INTERCONNECT_H

#include "connection.h"
#include "data.h"

#include <memory>
#include <vector>

namespace loom {

class Worker;
class DataBuilder;

class InterConnection : public SimpleConnectionCallback
{
public:
    InterConnection(Worker &worker);
    ~InterConnection();

    void send(std::shared_ptr<Data> &data);
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

    struct SendRecord {
        uv_write_t request;
        std::unique_ptr<char[]> raw_message;
        size_t raw_message_size;
        std::shared_ptr<Data> data;
    };

    void _send(SendRecord &record);

    void on_message(const char *buffer, size_t size);
    void on_data_chunk(const char *buffer, size_t size);
    void on_data_finish();
    void on_connection();
    void on_close();

    Worker &worker;
    std::string address;

    std::vector<std::unique_ptr<SendRecord>> send_records;
    std::unique_ptr<DataBuilder> data_builder;

    static void _on_write(uv_write_t *write_req, int status);

    static std::string make_address(const std::string &host, int port);
};

}

#endif // LOOM_INTERCONNECT_H
