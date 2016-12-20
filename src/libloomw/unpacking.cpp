
#include "unpacking.h"

using namespace loom;

DataUnpacker::~DataUnpacker()
{

}

DataUnpacker::Result DataUnpacker::get_initial_mode() {
   return MESSAGE;
}

DataUnpacker::Result DataUnpacker::on_message(const char *data, size_t size)
{
   assert(0);
}

DataUnpacker::Result DataUnpacker::on_stream_data(const char *data, size_t size, size_t remaining)
{
   assert(0);
}
