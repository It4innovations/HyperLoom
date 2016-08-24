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
    auto& config = task->get_config();
    std::shared_ptr<Data> output = std::make_shared<RawData>();
    RawData &data = static_cast<RawData&>(*output);
    memcpy(data.init_empty_file(worker, config.size()), config.c_str(), config.size());
    finish(output);
}


void MergeTask::start(DataVector &inputs) {
    size_t size = 0;
    for (auto& data : inputs) {
        size += (*data)->get_size();
    }
    std::shared_ptr<Data> output = std::make_shared<RawData>();
    RawData &data = static_cast<RawData&>(*output);
    data.init_empty_file(worker, size);
    char *dst = output->get_raw_data(worker);

    for (auto& data : inputs) {
        char *mem = (*data)->get_raw_data(worker);
        size_t size = (*data)->get_size();
        assert(mem || size == 0);
        memcpy(dst, mem, size);
        dst += size;
    }
    finish(output);
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
    auto input = *inputs[0];
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

