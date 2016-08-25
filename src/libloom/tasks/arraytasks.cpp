#include "arraytasks.h"
#include "libloom/compat.h"
#include "libloom/data/array.h"

using namespace loom;

void ArrayMakeTask::start(DataVector &inputs)
{
   size_t size =  inputs.size();
   auto items = std::make_unique<std::shared_ptr<Data>[]>(size);
   for (size_t i = 0; i < size; i++) {
      items[i] = *(inputs[i]);
   }
   std::shared_ptr<Data> output = std::make_shared<Array>(size, std::move(items));
   finish(output);
}
