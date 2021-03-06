#include "interconnect.h"
#include "worker.h"
#include "pb/comm.pb.h"
#include "libloom/log.h"
#include "libloom/pbutils.h"
#include "libloom/sendbuffer.h"

#include <sstream>

using namespace loom;
using namespace loom::base;

InterConnection::InterConnection(Worker &worker)
    : socket(worker.get_loop()), worker(worker), unpacking_data_id(-1), received_bytes(0)
{
    socket.set_on_close([this]() {
        logger->debug("Interconnection closed");
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
    logger->info("Connected to {}", get_address());
    loom::pb::comm::Announce msg;
    msg.set_port(worker.get_listen_port());

    send_message(socket, msg);

    for (auto& buffer : early_sends) {
        socket.send(std::move(buffer));
    }
    early_sends.clear();
}

void InterConnection::finish_receive()
{
    logger->debug("Interconnect: Data id={} received", unpacking_data_id);
    worker.data_transferred(unpacking_data_id);
    worker.publish_data(unpacking_data_id, unpacker->finish(), "");

    auto &trace = worker.get_trace();
    if (trace) {
        trace->trace_receive(unpacking_data_id, received_bytes, address);
    }

    unpacking_data_id = -1;
    received_bytes = 0;
    unpacker.reset();
}

void InterConnection::on_message(const char *buffer, size_t size)
{
    using namespace loom::pb::comm;
    if (!address.empty()) {
        if (unpacker) {
            received_bytes += size;
            // Message to unpacker
            auto result = unpacker->on_message(buffer, size);
            switch(result) {
            case DataUnpacker::FINISHED:
                finish_receive();
                return;
            case DataUnpacker::MESSAGE:
                return;
            case DataUnpacker::STREAM:
                socket.set_stream_mode(true);
                return;
            }
        } else {
            // First data message
            DataHeader msg;
            assert(msg.ParseFromArray(buffer, size));
            unpacking_data_id = msg.id();
            received_bytes = size;
            logger->debug("Interconnect: Receving data_id={}", unpacking_data_id);
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
        Announce msg;
        assert(msg.ParseFromArray(buffer, size));
        std::stringstream s;
        address = make_address(get_peername(), msg.port());
        logger->debug("Interconnection from worker {} accepted", address);
        worker.register_connection(*this);
    }
}

void InterConnection::on_stream_data(const char *buffer, size_t size, size_t remaining)
{
    received_bytes += size;
    assert(unpacker);
    auto result = unpacker->on_stream_data(buffer, size, remaining);
    switch(result) {
    case DataUnpacker::FINISHED:
        socket.set_stream_mode(false);
        finish_receive();
        return;
    case DataUnpacker::MESSAGE:
        socket.set_stream_mode(false);
        return;
    case DataUnpacker::STREAM:
        return;
    }
}

void InterConnection::send(Id id, DataPtr &data)
{
    auto buffer = std::make_unique<loom::base::SendBuffer>();

    size_t n_messages = data->serialize(worker, *buffer, data);

    loom::pb::comm::DataHeader msg;
    msg.set_id(id);
    msg.set_type_id(data->get_type_id(worker));
    msg.set_n_messages(n_messages);
    buffer->insert(0, loom::base::message_to_item(msg));

    auto &trace = worker.get_trace();
    if (trace) {
        trace->trace_send(id, buffer->get_data_size(), address);
    }

    auto state = socket.get_state();
    assert(state == loom::base::Socket::State::Open ||
           state == loom::base::Socket::State::Connecting);
    if (state == loom::base::Socket::State::Open) {
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
