#ifndef LIBLOOMW_PYOBJ_H
#define LIBLOOMW_PYOBJ_H

#include "../data.h"
#include "../unpacking.h"

#include <Python.h>

#include <mutex>

namespace loom {

class PyObj : public Data {

public:
    PyObj(PyObject *obj);
    ~PyObj();

    std::string get_type_name() const override;
    size_t get_size() const override;
    std::string get_info() const override;
    size_t serialize(Worker &worker, loom::base::SendBuffer &buffer, const DataPtr &data_ptr) const override;

    PyObject *unwrap() const {
        Py_IncRef(obj);
        return obj;
    }

protected:
    PyObject *obj;
    mutable size_t size;
};


class PyObjUnpacker : public DataUnpacker
{
public:
   PyObjUnpacker();
   Result on_message(const char *data, size_t size) override;
   DataPtr finish() override;
private:
   DataPtr result;
};

}

#endif // LIBLOOMW_PYOBJ_H
