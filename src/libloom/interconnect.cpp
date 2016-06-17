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
    assert(send_records.size() == 0);
}

void InterConnection::on_connection()
{
    llog->debug("Connected");
    connection.start_read();
    loomcomm::Announce msg;
    msg.set_port(worker.get_listen_port());
    send_message(msg);

    for (auto& record : send_records) {
        _send(*record);
    }
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

    size_t msg_size = msg.ByteSize();
    //std::cout << "MSG =" << msg_size << std::endl;
    auto msg_data = std::make_unique<char[]>(msg_size + sizeof(uint32_t));
    uint32_t *size_ptr = reinterpret_cast<uint32_t *>(msg_data.get());
    *size_ptr = msg_size;
    msg.SerializeToArray(msg_data.get() + sizeof(uint32_t), msg_size);

    auto record = std::make_unique<SendRecord>();
    record->request.data = this;
    record->raw_message = std::move(msg_data);
    record->raw_message_size = msg_size + sizeof(uint32_t);
    record->data = data;

    send_records.push_back(std::move(record));

    Connection::State state = connection.get_state();
    assert(state == Connection::ConnectionOpen ||
           state == Connection::ConnectionConnecting);
    if (state == Connection::ConnectionOpen) {
        _send(*send_records[send_records.size() - 1].get());
    }
}

void InterConnection::_send(InterConnection::SendRecord &record)
{
    llog->debug("Sending data id={} size={}",
                record.data->get_id(), record.data->get_size());
    uv_buf_t bufs[2];
    bufs[0].base = record.raw_message.get();
    bufs[0].len = record.raw_message_size;
    bufs[1] = record.data->get_uv_buf(worker);
    connection.send(&record.request, bufs, 2, _on_write);
}

void InterConnection::_on_write(uv_write_t *write_req, int status)
{
    UV_CHECK(status);
    InterConnection *connection = static_cast<InterConnection*>(write_req->data);
    assert(connection->send_records.size());
    connection->send_records.erase(connection->send_records.begin());
}

std::string InterConnection::make_address(const std::string &host, int port)
{
    std::stringstream s;
    s << host << ":" << port;
    return s.str();
}
