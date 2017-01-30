
#include "core.h"
#include "libloom/log.h"

#include "data_wrapper.h"
#include "module.h"

using namespace loom;
using namespace loom::base;

/** Ensures that python is initialized,
 *  if already initialized, then does nothing */
void loom::ensure_py_init() {
   static bool python_inited = false;
   if (python_inited) {
      return;
   }   
   logger->debug("Initializing Python interpreter");
   python_inited = true;
   PyImport_AppendInittab("loom_c", PyInit_loom_c);
   Py_Initialize();
   PyEval_InitThreads();

   data_wrapper_init();

   // Force the bootstrap sequence in MAIN thread
   PyObject *loom_wside = PyImport_ImportModule("loom.wside.core");
   Py_DecRef(loom_wside);

   PyEval_ReleaseLock();
   logger->debug("Python interpreter initialized");
}

PyObject* loom::deserialize_pyobject(const void *mem, size_t size)
{
   // Get cloudpickle
   PyObject *cloudpickle = PyImport_ImportModule("cloudpickle");
   if(!cloudpickle) {
      return nullptr;
   }

   PyObject *loads = PyObject_GetAttrString(cloudpickle, "loads");
   Py_DECREF(cloudpickle);

   if(!loads) {
      return nullptr;
   }

   assert(PyCallable_Check(loads));

   PyObject *data = PyBytes_FromStringAndSize(static_cast<const char*>(mem), size);
   assert(data);

   // call "call"
   PyObject *result = PyObject_CallFunctionObjArgs(loads, data, NULL);
   Py_DECREF(loads);
   Py_DECREF(data);
   return result;
}

std::shared_ptr<PyObj> loom::deserialize_pyobj(const void *mem, size_t size)
{
   PyObject *obj = deserialize_pyobject(mem, size);
   if (!obj) {
      return nullptr;
   }
   auto result = std::make_shared<PyObj>(obj);
   Py_DecRef(obj);
   return result;
}

size_t loom::get_sizeof(PyObject *obj)
{
    // Get getsizeof
    PyObject *getsizeof = PySys_GetObject("getsizeof");
    assert(getsizeof);

    // call getsizeof
    PyObject *result = PyObject_CallFunctionObjArgs(getsizeof, obj, NULL);

    if (!result) {
        PyErr_Clear();
        loom::base::logger->warn("Cannot determine size of a Python object");
        return 100;
    }

    size_t size = PyLong_AsSize_t(result);
    Py_DecRef(result);
    return size;
}
