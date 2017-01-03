#include "rawdatatasks.h"

#include "libloom/compat.h"
#include "libloomw/data/rawdata.h"
#include "libloomw/data/index.h"
#include "libloomw/data/externfile.h"
#include "libloom/log.h"
#include "libloomw/worker.h"

#include <string.h>

using namespace loom;


void ConstTask::start(DataVector &inputs)
{
    auto output = std::make_shared<RawData>();
    output->init_from_string(worker.get_work_dir(), task->get_config());
    finish(std::static_pointer_cast<Data>(output));
}

/** If there are more then 50 input or size is bigger then 200kB,
 *  then merge task is run in thread */
bool MergeJob::check_run_in_thread()
{
    const size_t SIZE_LIMIT = 200 * 1024;

    if (inputs.size() > 50) {
        return true;
    }
    size_t size = 0;
    for (auto& data : inputs) {
        size += data->get_size();
        if (size > SIZE_LIMIT) {
            return true;
        }
    }
    return false;
}

DataPtr MergeJob::run() {
    size_t size = 0;
    for (auto& data : inputs) {
        size += data->get_size();
    }

    const std::string &config = task.get_config();

    if (inputs.size() > 1) {
        size += (inputs.size() - 1) * config.size();
    }

    auto data = std::make_shared<RawData>();
    char *dst = data->init_empty(work_dir, size);

    if (config.empty()) {
        for (auto& data : inputs) {
            const char *mem = data->get_raw_data();
            size_t size = data->get_size();
            assert(mem || size == 0);
            memcpy(dst, mem, size);
            dst += size;
        }
    } else {
        bool first = true;
        for (auto& data : inputs) {

            if (first) {
                first = false;
            } else {
                memcpy(dst, config.c_str(), config.size());
                dst += config.size();
            }

            const char *mem = data->get_raw_data();
            size_t size = data->get_size();
            assert(mem || size == 0);
            memcpy(dst, mem, size);
            dst += size;
        }
    }
    return data;
}

void OpenTask::start(DataVector &inputs)
{
    DataPtr data = std::make_shared<ExternFile>(task->get_config());
    finish(data);
}

void SplitTask::start(DataVector &inputs)
{
    assert(inputs.size() == 1);
    char split_char = '\n';

    std::vector<size_t> indices;
    auto& input = inputs[0];
    const char *ptr = input->get_raw_data();
    size_t size = input->get_size();

    indices.push_back(0);
    for (size_t i = 0; i < size - 1; i++) {
        if (ptr[i] == split_char) {
            indices.push_back(i + 1);
        }
    }
    indices.push_back(size);

    auto indices_data = std::make_unique<size_t[]>(indices.size());
    memcpy(&indices_data[0], &indices[0], sizeof(size_t) * indices.size());
    DataPtr result = std::make_shared<Index>(worker, input, indices.size() - 1, std::move(indices_data));
    finish(result);
}

