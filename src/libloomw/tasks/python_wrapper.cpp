#include "python_wrapper.h"
using namespace loom;

static void
data_wrapper_dealloc(DataWrapper* self)
{
    self->data.~shared_ptr();
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
data_wrapper_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    DataWrapper *self;

    self = (DataWrapper *)type->tp_alloc(type, 0);
    if (self != NULL) {
        new (&self->data) std::shared_ptr<loom::Data>();
    }

    return (PyObject *)self;
}

static PyObject *
data_wrapper_size(DataWrapper* self)
{
    assert(self->data);
    return PyLong_FromUnsignedLong(self->data->get_size());
}

static PyObject *
data_wrapper_read(DataWrapper* self)
{
    assert(self->data);
    size_t size = self->data->get_size();
    const char *ptr = self->data->get_raw_data();
    return PyBytes_FromStringAndSize(ptr, size);
}

static Py_ssize_t
data_wrapper_len(DataWrapper* self)
{
    assert(self->data);
    return self->data->get_length();
}

#include <libloom/log.h>

static PyObject*
data_wrapper_get_item(DataWrapper *self, Py_ssize_t index)
{
    assert(self->data);

    if (index < 0 || index >= static_cast<Py_ssize_t>(self->data->get_length())) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return nullptr;
    }
    DataPtr data = self->data->get_at_index(index);
    assert(data);
    return (PyObject*) data_wrapper_create(data);
}

static PyMethodDef data_wrapper_methods[] = {
    {"size", (PyCFunction)data_wrapper_size, METH_NOARGS,
     "Return the size of the data object"
    },
    {"read", (PyCFunction)data_wrapper_read, METH_NOARGS,
     "Return byte representation of data object"
    },
    {NULL}  /* Sentinel */
};

static PySequenceMethods data_wrapper_sequence_methods = {
    (lenfunc) data_wrapper_len,
    0,
    0,
    (ssizeargfunc) data_wrapper_get_item,
};

static PyTypeObject data_wrapper_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Data",                    /* tp_name */
    sizeof(DataWrapper),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) data_wrapper_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    &data_wrapper_sequence_methods, /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "DataObject",              /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    data_wrapper_methods,      /* tp_methods */
    0,
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    data_wrapper_new,          /* tp_new */
};

void data_wrapper_init()
{
    assert(!PyType_Ready(&data_wrapper_type));
}

DataWrapper *data_wrapper_create(const loom::DataPtr &data)
{
    DataWrapper *self = (DataWrapper*) PyObject_CallFunctionObjArgs((PyObject*) &data_wrapper_type, NULL);
    assert(self);
    self->data = data;
    return self;
}

bool is_data_wrapper(PyObject *obj)
{
    return Py_TYPE(obj) == &data_wrapper_type;
}
