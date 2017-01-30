
#include "pyobj.h"

#include "libloom/compat.h"
#include "../python/core.h"
#include "libloom/log.h"

loom::PyObj::PyObj(PyObject *obj) : obj(obj), size(0)
{
    Py_IncRef(obj);
}

loom::PyObj::~PyObj()
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    assert(obj->ob_refcnt == 1);
    Py_DecRef(obj);

    PyGILState_Release(gstate);
}

std::string loom::PyObj::get_type_name() const
{
    return "loom/pyobj";
}

size_t loom::PyObj::get_size() const
{
    if (size == 0) {
        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();
        size = get_sizeof(obj);
        PyGILState_Release(gstate);
        return size;
    }
    return size;
}

std::string loom::PyObj::get_info() const
{
    return "PyObj";
}

size_t loom::PyObj::serialize(loom::Worker &worker, loom::base::SendBuffer &buffer, const loom::DataPtr &data_ptr) const
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    // Get cloudpickle
    PyObject *cloudpickle = PyImport_ImportModule("cloudpickle");
    assert(cloudpickle);

    // Get cloudpickle.dumps
    PyObject *dumps = PyObject_GetAttrString(cloudpickle, "dumps");
    Py_DECREF(cloudpickle);
    assert(dumps);

    // Call cloudpickle.dumps
    PyObject *result = PyObject_CallFunctionObjArgs(dumps, obj, NULL);
    Py_DECREF(dumps);
    assert(result);
    assert(PyBytes_Check(result));

    Py_ssize_t size = PyBytes_GET_SIZE(result);
    char *ptr = PyBytes_AsString(result);
    assert(ptr);

    auto item = std::make_unique<loom::base::MemItemWithSz>(size);
    memcpy(item->get_ptr(), ptr, size);
    buffer.add(std::move(item));

    Py_DECREF(result);
    PyGILState_Release(gstate);
    return 1;
}

loom::PyObjUnpacker::PyObjUnpacker()
{

}

loom::DataUnpacker::Result loom::PyObjUnpacker::on_message(const char *data, size_t size)
{
    ensure_py_init();
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    result = deserialize_pyobj(data, size);
    PyGILState_Release(gstate);
    return Result::FINISHED;
}

loom::DataPtr loom::PyObjUnpacker::finish()
{
    return std::move(result);
}
