#include "basictasks.h"

#include "libloom/databuilder.h"
#include "libloom/data/rawdata.h"
#include "libloom/data/externfile.h"

#include <string.h>

using namespace loom;

ConstTask::ConstTask(Worker &worker, std::unique_ptr<Task> task)
    : TaskInstance(worker, std::move(task))
{

}

void ConstTask::start(DataVector &inputs)
{
    auto& config = task->get_config();
    auto output = std::make_unique<RawData>();
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
    auto output = std::make_unique<RawData>();
    output->init_empty_file(worker, size);
    char *dst = output->get_raw_data(worker);

    for (auto& data : inputs) {
        char *mem = (*data)->get_raw_data(worker);
        assert(mem);
        size_t size = (*data)->get_size();
        memcpy(dst, mem, size);
        dst += size;
    }
    finish(std::move(output));
}

OpenTask::OpenTask(Worker &worker, std::unique_ptr<Task> task)
    : TaskInstance(worker, std::move(task))
{

}

void OpenTask::start(DataVector &inputs)
{
    finish(std::make_unique<ExternFile>(task->get_config()));
}
