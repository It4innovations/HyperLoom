#ifndef LIBLOOM_TASKS_PYTHON_H
#define LIBLOOM_TASKS_PYTHON_H

#include "libloomw/threadjob.h"

namespace loom {

class PyCallJob : public loom::ThreadJob
{
public:
    PyCallJob(Worker &worker, Task &task);

    void start(loom::DataVector &inputs);
    std::shared_ptr<loom::Data> run();
private:
    void set_python_error();
};

}


#endif // LIBLOOM_TASKS_PYTHON_H
