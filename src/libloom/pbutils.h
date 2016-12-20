#ifndef LOOM_SERVER_UTILS_H
#define LOOM_SERVER_UTILS_H

#include "sendbuffer.h"
#include "socket.h"

namespace google {
   namespace protobuf {
      class MessageLite;
   }
}

namespace loom {
namespace base {

std::unique_ptr<loom::base::SendBufferItem>
message_to_item(::google::protobuf::MessageLite &msg);

void send_message(loom::base::Socket &socket, ::google::protobuf::MessageLite &msg);

}}


#endif // LOOM_SERVER_UTILS_H
