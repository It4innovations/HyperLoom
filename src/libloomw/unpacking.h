
#ifndef LIBLOOMW_UNPACKING_H
#define LIBLOOMW_UNPACKING_H

#include "data.h"

#include <memory>

namespace loom {

class DataUnpacker {

public:
   enum Result {
      FINISHED,
      MESSAGE,
      STREAM
   };

   virtual ~DataUnpacker();

   virtual Result get_initial_mode();
   virtual Result on_message(const char *data, size_t size);
   virtual Result on_stream_data(const char *data, size_t size, size_t remaining);
   virtual DataPtr finish() = 0;
};

using UnpackFactoryFn = std::function<std::unique_ptr<DataUnpacker>()>;

}

#endif // LIBLOOMW_UNPACKING_H
