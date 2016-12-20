
#include "python.h"
#include "../data/rawdata.h"
#include "../log.h"
#include "libloomnet/compat.h"
#include "../worker.h"
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


PyCallJob::PyCallJob(Worker &worker, Task &task)
    : ThreadJob(worker, task)
{
    ensure_py_init();
}

static PyObject* data_vector_to_list(const DataVector &data)
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

static DataVector list_to_data_vector(PyObject *obj)
{
    assert(PySequence_Check(obj));
    size_t size = PySequence_Size(obj);
    assert(PyErr_Occurred() == nullptr);

    DataVector result;
    result.reserve(size);

    for (size_t i = 0; i < size; i++) {
        PyObject *o = PySequence_GetItem(obj, i);
        assert(o);
        assert(is_data_wrapper(o));
        DataWrapper *data = (DataWrapper*) o;
        result.push_back(data->data);
        Py_DecRef(o);
    }
    return result;
}

static bool is_task(PyObject *obj)
{
    return (PyObject_HasAttrString(obj, "task_type") &&
            PyObject_HasAttrString(obj, "config") &&
            PyObject_HasAttrString(obj, "inputs"));
}

static std::string get_attr_string(PyObject *obj, const char *name)
{
    PyObject *value = PyObject_GetAttrString(obj, name);
    assert(value);
    Py_ssize_t size;
    char *ptr;
    if (PyUnicode_Check(value)) {
        ptr = PyUnicode_AsUTF8AndSize(value, &size);
        assert(ptr);
    } else if (PyBytes_Check(value)) {
        size = PyBytes_GET_SIZE(value);
        ptr = PyBytes_AsString(value);
        assert(ptr);
    } else {
        assert(0);
    }
    std::string result(ptr, size);
    Py_DecRef(value);
    return result;
}

std::shared_ptr<Data> PyCallJob::run()
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
               task.get_config().c_str(),
               task.get_config().size());
   assert(config_data);

   PyObject *py_inputs = data_vector_to_list(inputs);
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
       output->init_from_mem(work_dir, ptr, size);

       Py_DECREF(result);
       PyGILState_Release(gstate);
       return output;
   } else if (PyBytes_Check(result)) {
       // Result is bytes
       Py_ssize_t size = PyBytes_GET_SIZE(result);
       char *ptr = PyBytes_AsString(result);
       assert(ptr);

       auto output = std::make_shared<RawData>();
       output->init_from_mem(work_dir, ptr, size);

       Py_DECREF(result);
       PyGILState_Release(gstate);
       return output;
   } else if (is_task(result)) {
       // Result is task
       auto task_desc = std::make_unique<TaskDescription>();
       task_desc->task_type = get_attr_string(result, "task_type");
       task_desc->config = get_attr_string(result, "config");

       PyObject *value = PyObject_GetAttrString(result, "inputs");
       assert(value);
       task_desc->inputs = list_to_data_vector(value);
       Py_DECREF(value);

       set_redirect(std::move(task_desc));
       Py_DECREF(result);
       PyGILState_Release(gstate);
       return nullptr;
   } else {
       set_error("Invalid result from python code");

       Py_DECREF(result);
       PyGILState_Release(gstate);
       return nullptr;
   }
}

void PyCallJob::set_python_error()
{
   loom::llog->error("Python error in task id={}", task.get_id());
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
