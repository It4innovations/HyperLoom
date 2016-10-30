#ifndef LIBLOOM_TASKS_PYTHON_H
#define LIBLOOM_TASKS_PYTHON_H

#include "libloom/ttinstance.h"

namespace loom {

class PyCallTask : public loom::ThreadTaskInstance
{
public:
    using ThreadTaskInstance::ThreadTaskInstance;
    void start(loom::DataVector &inputs);
    std::shared_ptr<loom::Data> run();
private:
    void set_python_error();
};

}


#endif // LIBLOOM_TASKS_PYTHON_H
