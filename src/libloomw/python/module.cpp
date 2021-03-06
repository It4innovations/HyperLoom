#include "module.h"
#include "libloom/log.h"
#include "../data/pyobj.h"
#include "data_wrapper.h"

static PyObject *
log_debug(PyObject *self, PyObject *args)
{
    const char *string;

    if (!PyArg_ParseTuple(args, "s", &string)) {
        return NULL;
    }

    loom::base::logger->debug(string);

    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
log_info(PyObject *self, PyObject *args)
{
    const char *string;

    if (!PyArg_ParseTuple(args, "s", &string)) {
        return NULL;
    }

    loom::base::logger->info(string);

    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
log_error(PyObject *self, PyObject *args)
{
    const char *string;

    if (!PyArg_ParseTuple(args, "s", &string)) {
        return NULL;
    }

    loom::base::logger->error(string);

    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
log_warn(PyObject *self, PyObject *args)
{
    const char *string;

    if (!PyArg_ParseTuple(args, "s", &string)) {
        return NULL;
    }

    loom::base::logger->warn(string);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
log_critical(PyObject *self, PyObject *args)
{
    const char *string;

    if (!PyArg_ParseTuple(args, "s", &string)) {
        return NULL;
    }

    loom::base::logger->critical(string);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
wrap(PyObject *self, PyObject *args)
{
    PyObject *obj;

    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }
    assert(obj);

    if (is_data_wrapper(obj)) {
        Py_IncRef(obj);
        return obj;
    }
    return (PyObject*) data_wrapper_create(std::make_shared<loom::PyObj>(obj));

}

static PyMethodDef loom_methods[] = {
    {"log_debug", log_debug, METH_VARARGS, ""},
    {"log_info", log_info, METH_VARARGS, ""},
    {"log_error", log_error, METH_VARARGS, ""},
    {"log_warn", log_warn, METH_VARARGS, ""},
    {"log_critical", log_critical, METH_VARARGS, ""},
    {"wrap", wrap, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef loom_c_module = {
   PyModuleDef_HEAD_INIT,
   "loom_c",   /* name of module */
   NULL,
   -1,
   loom_methods
};

PyMODINIT_FUNC
PyInit_loom_c(void)
{
    return PyModule_Create(&loom_c_module);
}
