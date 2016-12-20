#include "pbutils.h"
#include "libloomw/loomcomm.pb.h"

#include "compat.h"

std::unique_ptr<loom::net::SendBufferItem> loom::net::message_to_item(google::protobuf::MessageLite &msg)
{
      auto size = msg.ByteSize();
      auto item = std::make_unique<loom::net::MemItemWithSz>(size);
      msg.SerializeToArray(item->get_ptr(), size);
      return item;
}

void loom::net::send_message(loom::net::Socket &socket, google::protobuf::MessageLite &msg)
{
   auto buffer = std::make_unique<loom::net::SendBuffer>();
   buffer->add(message_to_item(msg));
   socket.send(std::move(buffer));
}
