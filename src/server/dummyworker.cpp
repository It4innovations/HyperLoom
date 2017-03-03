
#include "dummyworker.h"
#include "server.h"

#include <libloom/compat.h>
#include <libloom/pbutils.h>

#include <libloom/log.h>
#include <pb/comm.pb.h>


#include <sstream>
#include <assert.h>

using namespace loom;
using namespace loom::base;

DummyWorker::DummyWorker(Server &server)
   : server(server)
{

}

void DummyWorker::start_listen()
{
   uv_loop_t *loop = server.get_loop();
   if (loop == nullptr) {
      return;
   }
   listener.start(loop, 0, [this]() {
      auto connection = std::make_unique<DWConnection>(*this);
      connection->accept(listener);
      logger->debug("Worker data connection from {}", connection->get_peername());
      connections.push_back(std::move(connection));
   });
}

std::string DummyWorker::get_address() const
{
   std::stringstream s;
   s << "!:" << get_listen_port();
   return s.str();
}

DWConnection::DWConnection(DummyWorker &worker)
   : worker(worker), socket(worker.get_server().get_loop()), remaining_messages(0), registered(false)
{
   this->socket.set_on_close([this]() {
      logger->critical("Worker closing data connection from {}", this->socket.get_peername());
      assert(0);
   });

   this->socket.set_on_message([this](const char *buffer, size_t size) {
      on_message(buffer, size);
   });
}

DWConnection::~DWConnection()
{

}

void DWConnection::accept(loom::base::Listener &listener)
{
   listener.accept(socket);
}

void DWConnection::on_message(const char *buffer, size_t size)
{
   using namespace loom::pb::comm;
   if (!registered) {
      // This is first message: Announce, we do not care, so we drop it
      registered = true;
      return;
   }

   if (remaining_messages) {
      auto item = std::make_unique<base::MemItemWithSz>(size);
      memcpy(item->get_ptr(), buffer, size);
      send_buffer->add(std::move(item));

      remaining_messages--;
      if (remaining_messages == 0) {
         logger->debug("DummyWorker: Resending data to client");
         auto& server = worker.get_server();
         assert(server.has_client_connection());
         server.get_client_connection().send(std::move(send_buffer));
      }
      return;
   }

   assert(!send_buffer);
   send_buffer = std::make_unique<loom::base::SendBuffer>();

   DataHeader msg;
   assert(msg.ParseFromArray(buffer, size));

   remaining_messages = msg.n_messages();

   auto data_id = msg.id();
   auto client_id = worker.server.get_task_manager().pop_result_client_id(data_id);
   msg.set_id(client_id);
   logger->debug("DummyWorker: Capturing data for client data_id={} (messages={})", data_id, remaining_messages);

   ClientResponse cmsg;
   cmsg.set_type(ClientResponse_Type_DATA);
   *cmsg.mutable_data() = msg;
   send_buffer->add(base::message_to_item(cmsg));
}
