#ifndef LIBLOOM_TASKS_PYTHON_WRAPPER_H
#define LIBLOOM_TASKS_PYTHON_WRAPPER_H

#include "../data/rawdata.h"

#include <Python.h>

typedef struct {
    PyObject_HEAD
    loom::DataPtr data;
} DataWrapper;

void data_wrapper_init();
bool is_data_wrapper(PyObject *obj);
DataWrapper *data_wrapper_create(const loom::DataPtr &data);

#endif // LIBLOOM_TASKS_PYTHON_WRAPPER_H
