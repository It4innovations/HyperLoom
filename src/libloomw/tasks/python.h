#ifndef LIBLOOM_TASKS_PYTHON_H
#define LIBLOOM_TASKS_PYTHON_H

#include "libloomw/threadjob.h"
#include <Python.h>

namespace loom {

class PyBaseJob : public loom::ThreadJob
{
public:
   PyBaseJob(Worker &worker, Task &task);
protected:
   void set_python_error();
};

class PyCallJob : public PyBaseJob
{
public:
    using PyBaseJob::PyBaseJob;
    DataPtr run() override;
private:    
    DataPtr convert_py_object(PyObject *obj);
    DataVector list_to_data_vector(PyObject *obj);
};


class PyValueJob : public PyBaseJob
{
public:
    using PyBaseJob::PyBaseJob;
    DataPtr run() override;
};

}


#endif // LIBLOOM_TASKS_PYTHON_H
