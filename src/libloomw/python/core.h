#ifndef LIBLOOM_PYTHON_CORE_H
#define LIBLOOM_PYTHON_CORE_H

#include "../data/pyobj.h"
#include <Python.h>

namespace loom {

void ensure_py_init();
PyObject* deserialize_pyobject(const void *mem, size_t size);
std::shared_ptr<PyObj> deserialize_pyobj(const void *mem, size_t size);
size_t get_sizeof(PyObject *obj);

void lock_gil_in_main_thread();
void release_gil_in_main_thread();
}

#endif // LIBLOOM_PYTHON_CORE_H
