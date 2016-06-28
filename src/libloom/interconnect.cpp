#include "interconnect.h"
#include "worker.h"
#include "loomcomm.pb.h"
#include "log.h"
#include "utils.h"
#include "databuilder.h"

#include <sstream>

using namespace loom;

InterConnection::InterConnection(Worker &worker)
    : SimpleConnectionCallback(worker.get_loop()), worker(worker)
{

}

InterConnection::~InterConnection()
{

}

void InterConnection::on_connection()
{
    llog->debug("Connected");
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

void InterConnection::on_message(const char *buffer, size_t size)
{
    if (address.size()) {
        loomcomm::Data msg;
        msg.ParseFromArray(buffer, size);
        auto id = msg.id();
        auto size = msg.size();
        llog->debug("Receiving data id={} size={}", id, size);
        assert(data_builder.get() == nullptr);
        bool map_file = !worker.get_work_dir().empty();
        data_builder = std::make_unique<DataBuilder>(worker, id, size, map_file);
        connection.set_raw_read(size);
    } else {
        loomcomm::Announce msg;
        msg.ParseFromArray(buffer, size);
        std::stringstream s;
        address = make_address(connection.get_peername(), msg.port());
        llog->debug("Interconnection from worker {} accepted", address);
        worker.register_connection(*this);
    }
}

void InterConnection::on_data_chunk(const char *buffer, size_t size)
{
    assert(data_builder.get());
    data_builder->add(buffer, size);
}

void InterConnection::on_data_finish()
{
    std::unique_ptr<Data> data = data_builder->release_data();
    data_builder.reset();
    llog->debug("Data {} sucessfully received", data->get_id());
    assert(data.get());
    worker.publish_data(std::move(data));
}

void InterConnection::send(std::shared_ptr<Data> &data)
{
    loomcomm::Data msg;

    msg.set_id(data->get_id());
    msg.set_size(data->get_size());


    SendBuffer *buffer = new SendBuffer();
    buffer->add(msg);
    buffer->add(data, data->get_data(worker), data->get_size());

    Connection::State state = connection.get_state();
    assert(state == Connection::ConnectionOpen ||
           state == Connection::ConnectionConnecting);
    if (state == Connection::ConnectionOpen) {
        connection.send_buffer(buffer);
    } else {
        early_sends.push_back(std::unique_ptr<SendBuffer>(buffer));
    }
}

void InterConnection::send(std::unique_ptr<SendBuffer> buffer)
{

}

std::string InterConnection::make_address(const std::string &host, int port)
{
    std::stringstream s;
    s << host << ":" << port;
    return s.str();
}
