
#include "basictasks.h"

//#include "libloom/log.h"

using namespace loom;

void GetTask::start(loom::DataVector &inputs)
{
   assert(inputs.size() == 1);
   assert(task->get_config().size() == sizeof(size_t));
   const size_t *index = reinterpret_cast<const size_t*>(task->get_config().data());
   std::shared_ptr<Data> &input = *(inputs[0]);
   auto result = input->get_at_index(*index);
   finish(result);
}

void SliceTask::start(DataVector &inputs)
{
    assert(inputs.size() == 1);
    assert(task->get_config().size() == sizeof(size_t) * 2);
    const size_t *index = reinterpret_cast<const size_t*>(task->get_config().data());
    std::shared_ptr<Data> &input = *(inputs[0]);
    auto result = input->get_slice(index[0], index[1]);
    finish(result);
}
