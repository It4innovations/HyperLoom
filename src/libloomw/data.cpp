#include "data.h"
#include "worker.h"

using namespace loom;

Data::~Data() {

}

size_t Data::get_length() const
{
    return 0;
}

std::shared_ptr<Data> Data::get_at_index(size_t index) const
{
    assert(0);
}

std::shared_ptr<Data> Data::get_slice(size_t from, size_t to) const
{
    assert(0);
}

const char * Data::get_raw_data() const
{
    return nullptr;
}

std::string Data::get_filename() const
{
    return "";
}

bool Data::has_raw_data() const
{
   return false;
}

base::Id Data::get_type_id(Worker &worker) const
{
   return worker.get_dictionary().find_symbol(get_type_name());
}

DataBufferItem::DataBufferItem(std::shared_ptr<Data> &data, const char *mem, size_t size)
   : mem(mem), size(size), data(data)
{

}

uv_buf_t DataBufferItem::get_buf()
{
   uv_buf_t buf;
   buf.base = const_cast<char*>(mem);
   buf.len = size;
   return buf;
}
