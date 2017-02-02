
#include "resalloc.h"

#include <sstream>

std::vector<std::string> loom::taskset_command(const std::vector<int> cpus)
{
    std::vector<std::string> result;
    if (cpus.empty()) {
        return result;
    }

    result.reserve(3);
    result.push_back("taskset");
    result.push_back("-c");

    std::stringstream list;
    list << cpus[0];
    for (auto it = cpus.begin() + 1;  it != cpus.end(); it++) {
        list << "," << *it;
    }
    result.push_back(list.str());
    return result;
}

loom::ResourceAllocation::ResourceAllocation() : valid(false)
{

}
