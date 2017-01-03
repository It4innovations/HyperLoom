
#include "basetasks.h"
#include "../data/rawdata.h"
#include "../worker.h"

//#include "libloom/log.h"

using namespace loom;

void GetTask::start(DataVector &inputs)
{
   assert(inputs.size() == 1);
   assert(task->get_config().size() == sizeof(size_t));
   const size_t *index = reinterpret_cast<const size_t*>(task->get_config().data());
   DataPtr &input = inputs[0];
   auto result = input->get_at_index(*index);
   finish(result);
}

void SliceTask::start(DataVector &inputs)
{
    assert(inputs.size() == 1);
    assert(task->get_config().size() == sizeof(size_t) * 2);
    const size_t *index = reinterpret_cast<const size_t*>(task->get_config().data());
    DataPtr &input = inputs[0];
    auto result = input->get_slice(index[0], index[1]);
    finish(result);
}

void SizeTask::start(DataVector &inputs)
{
    size_t size = 0;
    for (auto &d : inputs) {
        size += d->get_size();
    }
    auto output = std::make_shared<RawData>();
    RawData &data = static_cast<RawData&>(*output);
    memcpy(data.init_empty(worker.get_work_dir(), sizeof(size_t)), &size, sizeof(size_t));
    finish(output);
}

void LengthTask::start(DataVector &inputs)
{
    size_t length = 0;
    for (auto &d : inputs) {
        length += d->get_length();
    }
    auto output = std::make_shared<RawData>();
    RawData &data = static_cast<RawData&>(*output);
    memcpy(data.init_empty(worker.get_work_dir(), sizeof(size_t)), &length, sizeof(size_t));
    finish(output);
}
