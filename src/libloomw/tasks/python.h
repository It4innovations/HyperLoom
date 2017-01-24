#ifndef LIBLOOM_TASKS_PYTHON_H
#define LIBLOOM_TASKS_PYTHON_H

#include "libloomw/threadjob.h"
#include <Python.h>

namespace loom {

class PyCallJob : public loom::ThreadJob
{
public:
    PyCallJob(Worker &worker, Task &task);

    DataPtr run() override;
private:
    void set_python_error();
    DataPtr convert_py_object(PyObject *obj);
    DataVector list_to_data_vector(PyObject *obj);
};

}


#endif // LIBLOOM_TASKS_PYTHON_H
