
#include "python.h"
#include "libloom/log.h"
#include "libloom/compat.h"

#include "../worker.h"
#include "../data/rawdata.h"
#include "../data/array.h"

#include "../python/core.h"
#include "../python/data_wrapper.h"


using namespace loom;
using namespace loom::base;

static PyObject* data_vector_to_list(DataVector::const_iterator begin,
                                     DataVector::const_iterator end)
{
    PyObject *list = PyTuple_New(end - begin);
    assert(list);
    size_t i = 0;
    for (auto it = begin; it != end; it++) {
        PyTuple_SET_ITEM(list, i, (PyObject*) data_wrapper_create(*it));
        i++;
    }
    return list;
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

DataVector PyCallJob::list_to_data_vector(PyObject *obj)
{
    assert(PySequence_Check(obj));
    size_t size = PySequence_Size(obj);
    assert(PyErr_Occurred() == nullptr);

    DataVector result;
    result.reserve(size);

    for (size_t i = 0; i < size; i++) {
        PyObject *o = PySequence_GetItem(obj, i);
        assert(o);
        result.push_back(convert_py_object((o)));
        Py_DecRef(o);
    }
    return result;
}


DataPtr PyCallJob::convert_py_object(PyObject *obj)
{
    assert(obj);
    if (is_data_wrapper(obj)) {
        DataWrapper *data = (DataWrapper*) obj;
        return data->data;
    } else if (PyUnicode_Check(obj)) {
        // obj is string
        Py_ssize_t size;
        char *ptr = PyUnicode_AsUTF8AndSize(obj, &size);
        assert(ptr);
        auto output = std::make_shared<RawData>();
        output->init_from_mem(globals, ptr, size);
        return output;
    } else if (PyBytes_Check(obj)) {
        // obj is bytes
        Py_ssize_t size = PyBytes_GET_SIZE(obj);
        char *ptr = PyBytes_AsString(obj);
        assert(ptr);

        auto output = std::make_shared<RawData>();
        output->init_from_mem(globals, ptr, size);
        return output;
    } else if (PySequence_Check(obj)) {
        DataVector vector = list_to_data_vector(obj);
        return std::make_shared<Array>(vector);
    }
    return nullptr;
}

DataPtr PyCallJob::run()
{
   assert(!inputs.empty());
   DataPtr fn = std::move(inputs[0]);
   inputs.erase(inputs.begin());

   if (fn->get_type_name() != "loom/pyobj") {
        set_error("The first argument is not PyObj");
        return nullptr;
   }

   // Obtain GIL
   PyGILState_STATE gstate;
   gstate = PyGILState_Ensure();

   // Get loom.wside
   PyObject *loom_wep_call = PyImport_ImportModule("loom.wside.core");
   if(!loom_wep_call) {
      set_python_error();
      PyGILState_Release(gstate);
      return nullptr;
   }

   PyObject *call_fn = PyObject_GetAttrString(loom_wep_call, "execute");
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

   PyObject *py_inputs = data_vector_to_list(inputs.begin(), inputs.end());
   assert(py_inputs);

   PyObject *task_id = PyLong_FromLong(task.get_id());
   assert(task_id);

   PyObject *fn_obj = std::static_pointer_cast<const PyObj>(fn)->unwrap();
   assert(fn_obj);

   PyObject *result = PyObject_CallFunctionObjArgs(call_fn, fn_obj, config_data, py_inputs, task_id, NULL);
   Py_DECREF(fn_obj);
   Py_DECREF(task_id);
   Py_DECREF(py_inputs);
   Py_DECREF(call_fn);
   Py_DECREF(config_data);

   if(!result) {
      set_python_error();
      PyGILState_Release(gstate);
      return nullptr;
   }

   if (is_task(result)) {
           // task redirection
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
   }

   DataPtr data = convert_py_object(result);
   Py_DECREF(result);
   PyGILState_Release(gstate);

   if (data == nullptr) {
        set_error("Invalid result from python code");
        return nullptr;
   }
   return data;
}

PyBaseJob::PyBaseJob(Worker &worker, Task &task)
   : ThreadJob(worker, task)
{

}

void PyBaseJob::set_python_error()
{
   logger->error("Python error in task id={}", task.get_id());
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

   logger->error("Python exception: {}", error_msg);

   set_error(std::string("Python exception:\n") + error_msg);

   Py_DECREF(str_obj);
   PyErr_Clear();
}

DataPtr PyValueJob::run()
{
   PyGILState_STATE gstate;
   gstate = PyGILState_Ensure();

   auto &config = task.get_config();
   DataPtr data = deserialize_pyobj(config.c_str(), config.size());

   if (data == nullptr)  {
      set_python_error();
      PyGILState_Release(gstate);
      return nullptr;
   }

   PyGILState_Release(gstate);
   return data;
}
