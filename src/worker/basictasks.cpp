#include "basictasks.h"

#include "libloom/databuilder.h"

#include <string.h>

using namespace loom;

ConstTask::ConstTask(Worker &worker, std::unique_ptr<Task> task)
    : TaskInstance(worker, std::move(task))
{

}

void ConstTask::start(DataVector &inputs) {
    auto& config = task->get_config();
    auto output = std::make_unique<Data>(get_id());
    memcpy(output->init_empty_file(worker, config.size()), config.c_str(), config.size());
    finish(std::move(output));
}

MergeTask::MergeTask(Worker &worker, std::unique_ptr<Task> task)
    : TaskInstance(worker, std::move(task))
{

}

void MergeTask::start(DataVector &inputs) {
    size_t size = 0;
    for (auto& data : inputs) {
        size += (*data)->get_size();
    }
    DataBuilder builder(worker, get_id(), size, true);
    for (auto& data : inputs) {
        builder.add((*data)->get_data(worker), (*data)->get_size());
    }
    finish(builder.release_data());
}
