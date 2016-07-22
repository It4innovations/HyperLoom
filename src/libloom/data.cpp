#include "data.h"
#include "worker.h"
#include "log.h"

using namespace loom;

Data::~Data() {

}

size_t Data::get_length() const
{
    return 0;
}

void Data::serialize(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr)
{
    loomcomm::Data msg;
    msg.set_type_id(get_type_id());
    msg.set_size(get_size());
    auto length = get_length();
    if (length) {
        msg.set_length(length);
    }
    init_message(worker, msg);
    buffer.add(msg);
    serialize_data(worker, buffer, data_ptr);
}

void Data::init_message(Worker &worker, loomcomm::Data &msg) const
{

}

char *Data::get_raw_data(Worker &worker)
{
    return nullptr;
}

std::string Data::get_filename() const
{
   return "";
}

DataUnpacker::~DataUnpacker()
{

}

bool DataUnpacker::on_message(Connection &connection, const char *data, size_t size)
{
    assert(0);
}

void DataUnpacker::on_data_chunk(const char *data, size_t size)
{
    assert(0);
}

bool DataUnpacker::on_data_finish(Connection &connection)
{
    assert(0);
}
