#include "arraytasks.h"
#include "libloom/compat.h"
#include "libloom/data/array.h"

using namespace loom;

void ArrayMakeTask::start(loom::DataVector &inputs)
{
   size_t size =  inputs.size();
   auto items = std::make_unique<std::shared_ptr<Data>[]>(size);
   for (size_t i = 0; i < size; i++) {
      items[i] = *(inputs[i]);
   }
   std::shared_ptr<Data> output = std::make_shared<Array>(size, std::move(items));
   finish(output);
}

void ArrayGetTask::start(loom::DataVector &inputs)
{
   assert(inputs.size() == 1);
   Array *array = dynamic_cast<Array*>((*inputs[0]).get());
   assert(array);

   assert(task->get_config().size() == sizeof(size_t));
   const size_t *index = reinterpret_cast<const size_t*>(task->get_config().data());
   finish(array->get_at_index(*index));
}
