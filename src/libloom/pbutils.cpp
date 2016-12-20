#include "pbutils.h"
#include "libloomw/loomcomm.pb.h"

#include "compat.h"

std::unique_ptr<loom::base::SendBufferItem> loom::base::message_to_item(google::protobuf::MessageLite &msg)
{
      auto size = msg.ByteSize();
      auto item = std::make_unique<loom::base::MemItemWithSz>(size);
      msg.SerializeToArray(item->get_ptr(), size);
      return item;
}

void loom::base::send_message(loom::base::Socket &socket, google::protobuf::MessageLite &msg)
{
   auto buffer = std::make_unique<loom::base::SendBuffer>();
   buffer->add(message_to_item(msg));
   socket.send(std::move(buffer));
}
