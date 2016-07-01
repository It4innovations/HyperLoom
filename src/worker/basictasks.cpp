#include "basictasks.h"

#include "libloom/databuilder.h"
#include "libloom/data/rawdata.h"
#include "libloom/data/externfile.h"

#include <string.h>

using namespace loom;


void ConstTask::start(DataVector &inputs)
{
    auto& config = task->get_config();
    auto output = std::make_unique<RawData>();
    memcpy(output->init_empty_file(worker, config.size()), config.c_str(), config.size());
    finish(std::move(output));
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

void OpenTask::start(DataVector &inputs)
{
    finish(std::make_unique<ExternFile>(task->get_config()));
}

void LineSplitTask::start(DataVector &inputs)
{
    assert(inputs.size() == 1);
    assert(sizeof(uint64_t) * 2 == task->get_config().size());
    const uint64_t *indices = reinterpret_cast<const uint64_t*>(task->get_config().c_str());
    uint64_t start = indices[0];
    uint64_t count = indices[1] - start;

    auto input = *inputs[0];

    char *start_ptr = input->get_raw_data(worker);
    char *end_ptr = start_ptr + input->get_size();

    if (start) {
        while (start_ptr != end_ptr) {
            if (*start_ptr == '\n') {
                start--;
                if (start == 0) {
                    start_ptr++;
                    break;
                }
            }
            start_ptr++;
        }
    }

    char *data_end = start_ptr;

    if (count) {
        while (data_end != end_ptr) {
            if (*data_end == '\n') {
                count--;
                if (count == 0) {
                    data_end++;
                    break;
                }
            }
            data_end++;
        }
    }
    size_t data_size = data_end - start_ptr;
    auto output = std::make_unique<RawData>();
    output->init_empty_file(worker, data_size);
    char *dst = output->get_raw_data(worker);
    memcpy(dst, start_ptr, data_size);
    finish(std::move(output));

}
