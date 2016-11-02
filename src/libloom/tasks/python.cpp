
#include "python.h"
#include "../data/rawdata.h"
#include "../log.h"
#include "python_wrapper.h"

#include <Python.h>

using namespace loom;

/** Ensures that python is initialized,
 *  if already initialized, then does nothing */
static void ensure_py_init() {
   static bool python_inited = false;
   if (python_inited) {
      return;
   }
   python_inited = true;
   Py_Initialize();
   PyEval_InitThreads();

   data_wrapper_init();

   PyEval_ReleaseLock();
}


void loom::PyCallTask::start(loom::DataVector &inputs)
{
   ensure_py_init();
   ThreadTaskInstance::start(inputs);
}

static PyObject* vector_of_data_to_list(const DataVector &data)
{
    PyObject *list = PyTuple_New(data.size());
    assert(list);
    size_t i = 0;
    for (auto& item : data) {
        PyTuple_SET_ITEM(list, i, (PyObject*) data_wrapper_create(item));
        i++;
    }
    return list;
}

std::shared_ptr<Data> PyCallTask::run()
{
   // Obtain GIL
   PyGILState_STATE gstate;
   gstate = PyGILState_Ensure();

   // Get loom.wep
   PyObject *loom_wep_call = PyImport_ImportModule("loom.wep.call");
   if(!loom_wep_call) {
      set_python_error();
      PyGILState_Release(gstate);
      return nullptr;
   }

   PyObject *call_fn = PyObject_GetAttrString(loom_wep_call, "unpack_and_execute");
   Py_DECREF(loom_wep_call);

   if(!call_fn) {
      set_python_error();
      PyGILState_Release(gstate);
      return nullptr;
   }
   assert(PyCallable_Check(call_fn));

   PyObject *config_data = PyBytes_FromStringAndSize(
               task->get_config().c_str(),
               task->get_config().size());
   assert(config_data);

   PyObject *py_inputs = vector_of_data_to_list(inputs);
   assert(py_inputs);


   // call "call"
   PyObject *result = PyObject_CallFunctionObjArgs(call_fn, config_data, py_inputs, NULL);
   Py_DECREF(call_fn);
   Py_DECREF(config_data);

   if(!result) {
      set_python_error();
      PyGILState_Release(gstate);
      return nullptr;
   }

   if (PyUnicode_Check(result)) {
       // Result is string
       Py_ssize_t size;
       char *ptr = PyUnicode_AsUTF8AndSize(result, &size);
       assert(ptr);

       auto output = std::make_shared<RawData>();
       output->init_from_mem(worker, ptr, size);

       Py_DECREF(result);
       PyGILState_Release(gstate);
       return output;
   } else if (PyBytes_Check(result)) {
       // Result is bytes
       Py_ssize_t size = PyBytes_GET_SIZE(result);
       char *ptr = PyBytes_AsString(result);
       assert(ptr);

       auto output = std::make_shared<RawData>();
       output->init_from_mem(worker, ptr, size);

       Py_DECREF(result);
       PyGILState_Release(gstate);
       return output;
   } else {
       set_error("Invalid result from python code");

       Py_DECREF(result);
       PyGILState_Release(gstate);
       return nullptr;
   }
}

void PyCallTask::set_python_error()
{
   loom::llog->error("Python error in task id={}", task->get_id());
   PyObject *excType, *excValue, *excTraceback;
   PyErr_Fetch(&excType, &excValue, &excTraceback);
   assert(excType);
   PyErr_NormalizeException(&excType, &excValue, &excTraceback);

   if (!excTraceback) {
      excTraceback = Py_None;
      Py_INCREF(excTraceback);
   }

   PyException_SetTraceback(excValue, excTraceback);


   PyObject *str_obj = PyObject_Str(excValue);
   assert(str_obj);
   PyObject_Print(excValue, stdout, 0);

   Py_ssize_t size;
   char *str = PyUnicode_AsUTF8AndSize(str_obj, &size);
   assert(str);
   std::string error_msg(str, size);

   loom::llog->error("Python exception: {}", error_msg);

   set_error(std::string("Python exception:\n") + error_msg);

   Py_DECREF(str_obj);
   PyErr_Clear();
}
