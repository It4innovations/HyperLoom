
#include "dummyworker.h"
#include "server.h"

#include <libloom/compat.h>
#include <libloom/utils.h>
#include <libloom/log.h>
#include <libloom/loomcomm.pb.h>
#include <libloom/sendbuffer.h>

#include <sstream>
#include <assert.h>

using namespace loom;

DummyWorker::DummyWorker(Server &server)
    : server(server), listen_port(-1)
{
    UV_CHECK(uv_tcp_init(server.get_loop(), &listen_socket));
    listen_socket.data = this;
}

void DummyWorker::start_listen()
{
    struct sockaddr_in addr;
    UV_CHECK(uv_ip4_addr("0.0.0.0", 0, &addr));

    UV_CHECK(uv_tcp_bind(&listen_socket, (const struct sockaddr *) &addr, 0));
    UV_CHECK(uv_listen((uv_stream_t *) &listen_socket, 10, _on_new_connection));

    struct sockaddr_in sockname;
    int namelen = sizeof(sockname);
    uv_tcp_getsockname(&listen_socket, (sockaddr*) &sockname, &namelen);
    listen_port = ntohs(sockname.sin_port);
}

std::string DummyWorker::get_address() const
{
    std::stringstream s;
    s << "!:" << get_listen_port();
    return s.str();

}

void DummyWorker::_on_new_connection(uv_stream_t *stream, int status)
{
    UV_CHECK(status);
    DummyWorker *worker = static_cast<DummyWorker*>(stream->data);
    auto connection = std::make_unique<DWConnection>(*worker);
    connection->accept(&worker->listen_socket);
    llog->debug("Worker data connection from {}", connection->get_peername());
    worker->connections.push_back(std::move(connection));
}

DWConnection::DWConnection(DummyWorker &worker)
    : SimpleConnectionCallback(worker.server.get_loop()), worker(worker), registered(false)
{

}

DWConnection::~DWConnection()
{

}

void DWConnection::on_message(const char *buffer, size_t size)
{
    if (!registered) {
        // This is first message: Announce, we do not care, so we drop it
        registered = true;
        return;
    }
    assert(this->send_buffer.get() == nullptr);

    send_buffer = std::make_unique<SendBuffer>();

    loomcomm::DataPrologue msg;
    msg.ParseFromArray(buffer, size);

    loomcomm::ClientMessage cmsg;
    cmsg.set_type(loomcomm::ClientMessage_Type_DATA);
    *cmsg.mutable_data() = msg;

    send_buffer->add(cmsg);

    assert(msg.has_data_size());
    size_t data_size = msg.data_size();
    llog->debug("Fetching data id={} data_size={}", msg.id(), data_size);

    auto mem = std::make_unique<char[]>(data_size);
    pointer = mem.get();
    send_buffer->add(std::move(mem), data_size);

    connection.set_raw_read(data_size);
}

void DWConnection::on_data_chunk(const char *buffer, size_t size)
{
    memcpy(pointer, buffer, size);
    pointer += size;
}

void DWConnection::on_data_finish()
{
    llog->debug("Resending data to client");
    worker.server.get_client_connection().send_buffer(send_buffer.release());
}

void DWConnection::on_close()
{
    llog->debug("Worker closing data connection from {}", connection.get_peername());
    auto& connections = worker.connections;
    for (auto i = connections.begin(); i != connections.begin(); i++) {
        if ((*i).get() == this) {
            connections.erase(i);
        }
    }
    assert(0);
}
