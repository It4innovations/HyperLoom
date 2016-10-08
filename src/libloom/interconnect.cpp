#include "interconnect.h"
#include "worker.h"
#include "loomcomm.pb.h"
#include "log.h"
#include "utils.h"

#include <sstream>

using namespace loom;

InterConnection::InterConnection(Worker &worker)
    : SimpleConnectionCallback(worker.get_loop()), worker(worker), data_id(-1)
{

}

InterConnection::~InterConnection()
{

}

void InterConnection::on_connection()
{
    llog->info("Connected to {}", get_address());
    connection.start_read();
    loomcomm::Announce msg;
    msg.set_port(worker.get_listen_port());
    send_message(msg);

    for (auto& buffer : early_sends) {
        connection.send_buffer(buffer.release());
    }
    early_sends.clear();
}

void InterConnection::on_close()
{
    llog->debug("Interconnection closed");
    worker.unregister_connection(*this);
}

void InterConnection::finish_data()
{
    llog->debug("Data {} sucessfully received", data_id);
    worker.publish_data(data_id,
                        data_unpacker->get_data());
    data_unpacker.reset();
    data_id = -1;
}

void InterConnection::on_message(const char *buffer, size_t size)
{
    if (data_unpacker.get()) {
        data_unpacker->on_message(connection, buffer, size);;
        return;
    }
    if (data_id > 0) {
        assert(data_unpacker.get() == nullptr);
        loomcomm::Data msg;
        assert(msg.ParseFromArray(buffer, size));
        data_unpacker = worker.unpack(msg.type_id());
        if (data_unpacker->init(worker, connection, msg)) {
            finish_data();
        }
        return;
    } else if (address.size()) {
        loomcomm::DataPrologue msg;
        assert(msg.ParseFromArray(buffer, size));
        auto id = msg.id();
        data_id = id;
        if (msg.has_data_size()) {
            llog->debug("Receiving data id={} (data_size={})", id, msg.data_size());
        } else {
            llog->debug("Receiving data id={}", id);
        }
    } else {
        // First message
        loomcomm::Announce msg;
        assert(msg.ParseFromArray(buffer, size));
        std::stringstream s;
        address = make_address(connection.get_peername(), msg.port());
        llog->debug("Interconnection from worker {} accepted", address);
        worker.register_connection(*this);
    }
}

void InterConnection::on_data_chunk(const char *buffer, size_t size)
{
    assert(data_unpacker.get());
    data_unpacker->on_data_chunk(buffer, size);
}

void InterConnection::on_data_finish()
{
    assert(data_unpacker.get());
    if (data_unpacker->on_data_finish(connection)) {
        finish_data();
    }
}

void InterConnection::send(Id id, std::shared_ptr<Data> &data, bool with_size)
{
    SendBuffer *buffer = new SendBuffer();
    loomcomm::DataPrologue msg;
    msg.set_id(id);

    if (!with_size) {
        buffer->add(msg);
    }

    data->serialize(worker, *buffer, data);

    if (with_size) {
        msg.set_data_size(buffer->get_size());
        buffer->insert(0, msg);
    }

    Connection::State state = connection.get_state();
    assert(state == Connection::ConnectionOpen ||
           state == Connection::ConnectionConnecting);
    if (state == Connection::ConnectionOpen) {
        connection.send_buffer(buffer);
    } else {
        early_sends.push_back(std::unique_ptr<SendBuffer>(buffer));
    }
}

std::string InterConnection::make_address(const std::string &host, int port)
{
    std::stringstream s;
    s << host << ":" << port;
    return s.str();
}
