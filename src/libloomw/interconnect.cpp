#include "interconnect.h"
#include "worker.h"
#include "loomcomm.pb.h"
#include "log.h"
#include "libloomnet/pbutils.h"
#include "libloomnet/sendbuffer.h"

#include <sstream>

using namespace loom;

InterConnection::InterConnection(Worker &worker)
    : socket(worker.get_loop()), worker(worker), unpacking_data_id(-1)
{
    socket.set_on_close([this]() {
        llog->debug("Interconnection closed");
        this->worker.unregister_connection(*this);
    });

    socket.set_on_connect([this]() {
        on_connect();
    });

    socket.set_on_message([this](const char *buffer, size_t size) {
        on_message(buffer, size);
    });

    socket.set_on_stream_data([this](const char *buffer, size_t size, size_t remaining) {
        on_stream_data(buffer, size, remaining);
    });
}

InterConnection::~InterConnection()
{

}

void InterConnection::on_connect()
{
    llog->info("Connected to {}", get_address());
    loomcomm::Announce msg;
    msg.set_port(worker.get_listen_port());

    send_message(socket, msg);

    for (auto& buffer : early_sends) {
        socket.send(std::move(buffer));
    }
    early_sends.clear();
}

void InterConnection::on_message(const char *buffer, size_t size)
{
    if (!address.empty()) {
        if (unpacker) {
            // Message to unpacker
            auto result = unpacker->on_message(buffer, size);
            switch(result) {
            case DataUnpacker::FINISHED:
                worker.publish_data(unpacking_data_id, unpacker->finish());
                unpacking_data_id = -1;
                unpacker.reset();
                return;
            case DataUnpacker::MESSAGE:
                return;
            case DataUnpacker::STREAM:
                socket.set_stream_mode(true);
                return;
            }
        } else {
            // First data message
            loomcomm::DataHeader msg;
            assert(msg.ParseFromArray(buffer, size));
            unpacking_data_id = msg.id();
            llog->debug("Interconnect: Receving data_id={}", unpacking_data_id);
            unpacker = worker.get_unpacker(msg.type_id());
            switch(unpacker->get_initial_mode()) {
                case DataUnpacker::MESSAGE:
                    // We are already in message mode
                    return;
                case DataUnpacker::STREAM:
                    socket.set_stream_mode(true);
                    return;
                default:
                    assert(0);
            }
        }
    } else {
        // First message
        loomcomm::Announce msg;
        assert(msg.ParseFromArray(buffer, size));
        std::stringstream s;
        address = make_address(get_peername(), msg.port());
        llog->debug("Interconnection from worker {} accepted", address);
        worker.register_connection(*this);
    }
}

void InterConnection::on_stream_data(const char *buffer, size_t size, size_t remaining)
{
    assert(unpacker);
    auto result = unpacker->on_stream_data(buffer, size, remaining);
    switch(result) {
    case DataUnpacker::FINISHED:
        worker.publish_data(unpacking_data_id, unpacker->finish());
        unpacking_data_id = -1;
        unpacker.reset();
        socket.set_stream_mode(false);
        return;
    case DataUnpacker::MESSAGE:
        socket.set_stream_mode(false);
        return;
    case DataUnpacker::STREAM:
        return;
    }
}

void InterConnection::send(Id id, std::shared_ptr<Data> &data)
{
    auto buffer = std::make_unique<loom::net::SendBuffer>();

    size_t n_messages = data->serialize(worker, *buffer, data);

    loomcomm::DataHeader msg;
    msg.set_id(id);
    msg.set_type_id(data->get_type_id(worker));
    msg.set_n_messages(n_messages);
    buffer->insert(0, loom::net::message_to_item(msg));

    auto state = socket.get_state();
    assert(state == loom::net::Socket::State::Open ||
           state == loom::net::Socket::State::Connecting);
    if (state == loom::net::Socket::State::Open) {
        socket.send(std::move(buffer));
    } else {
        early_sends.push_back(std::move(buffer));
    }
}

std::string InterConnection::make_address(const std::string &host, int port)
{
    std::stringstream s;
    s << host << ":" << port;
    return s.str();
}
