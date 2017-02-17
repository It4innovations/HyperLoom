#ifndef LIBLOOMW_INTERCONNECT_H
#define LIBLOOMW_INTERCONNECT_H

#include "libloom/socket.h"
#include "libloom/listener.h"
#include "data.h"
#include "unpacking.h"

#include <memory>
#include <vector>

namespace loom {

class Worker;
class DataBuilder;
class SendBuffer;

/** Interconnection between workers */
class InterConnection
{
public:
    InterConnection(Worker &worker);
    ~InterConnection();

    void send(base::Id id, DataPtr &data);

    std::string get_peername() {
        return socket.get_peername();
    }
    std::string get_address() const {
        return address;
    }

    void connect(const std::string &address, int port) {
        this->address = make_address(address, port);
        socket.connect(address, port);
    }

    void accept(loom::base::Listener &listener) {
        listener.accept(socket);
    }

    void close() {
        socket.close();
    }

protected:

    void _send(SendBuffer &buffer);

    void on_message(const char *buffer, size_t size);
    void on_stream_data(const char *buffer, size_t size, size_t remaining);
    void on_connect();

    void finish_receive();

    base::Socket socket;
    Worker &worker;
    std::string address;

    std::unique_ptr<DataUnpacker> unpacker;
    base::Id unpacking_data_id;
    size_t received_bytes;

    static std::string make_address(const std::string &host, int port);

    std::vector<std::unique_ptr<base::SendBuffer>> early_sends;
};

}

#endif // LIBLOOMW_INTERCONNECT_H
