#include "rawdatatasks.h"

#include "libloom/compat.h"
#include "libloom/data/rawdata.h"
#include "libloom/data/index.h"
#include "libloom/data/externfile.h"
#include "libloom/log.h"

#include <string.h>

using namespace loom;


void ConstTask::start(DataVector &inputs)
{
    auto output = std::make_shared<RawData>();
    output->init_from_string(worker, task->get_config());
    finish(std::static_pointer_cast<Data>(output));
}

/** If there are more then 50 input or size is bigger then 200kB,
 *  then merge task is run in thread */
bool MergeTask::run_in_thread(DataVector &input_data)
{
    const size_t SIZE_LIMIT = 200 * 1024;

    if (input_data.size() > 50) {
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

std::shared_ptr<Data> MergeTask::run() {
    size_t size = 0;
    for (auto& data : inputs) {
        size += data->get_size();
    }

    const std::string &config = task->get_config();
    if (inputs.size() > 1) {
        size += (inputs.size() - 1) * config.size();
    }

    std::shared_ptr<Data> output = std::make_shared<RawData>();
    RawData &data = static_cast<RawData&>(*output);
    data.init_empty(worker, size);
    char *dst = output->get_raw_data(worker);

    if (config.empty()) {
        for (auto& data : inputs) {
            char *mem = data->get_raw_data(worker);
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

            char *mem = data->get_raw_data(worker);
            size_t size = data->get_size();
            assert(mem || size == 0);
            memcpy(dst, mem, size);
            dst += size;
        }
    }
    return output;
}

void OpenTask::start(DataVector &inputs)
{
    std::shared_ptr<Data> data = std::make_shared<ExternFile>(task->get_config());
    finish(data);
}

void SplitTask::start(DataVector &inputs)
{
    assert(inputs.size() == 1);
    char split_char = '\n';

    std::vector<size_t> indices;
    auto& input = inputs[0];
    char *ptr = input->get_raw_data(worker);
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
    std::shared_ptr<Data> result = std::make_shared<Index>(worker, input, indices.size() - 1, std::move(indices_data));
    finish(result);
}

