#ifndef LIBLOOM_TASKS_PYTHON_WRAPPER_H
#define LIBLOOM_TASKS_PYTHON_WRAPPER_H

#include "../data/rawdata.h"

#include <Python.h>

typedef struct {
    PyObject_HEAD
    std::shared_ptr<loom::Data> data;
} DataWrapper;

void data_wrapper_init();
DataWrapper *data_wrapper_create(const std::shared_ptr<loom::Data> &data);

#endif // LIBLOOM_TASKS_PYTHON_WRAPPER_H
