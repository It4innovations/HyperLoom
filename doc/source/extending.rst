
Extending worker
================

.. warning::
   The API in the following section is not yet fully stable.
   It may be changed in the near future.

HyperLoom infrastructure offers by default a set of operations for basic manipulation
with data objects and running and external programs. One of this task is also
task `loom/py_call` (it can be used via ``tasks.py_call`` or ``tasks.py_task``
in Python client). This task allows to executed arbitrary Python codes and the
user may define new tasks.

The another way is to directly extend a worker itself. The primary purpose is
efficiency, since worker extensions can be written in C++. Moreover, this
approach is more powerfull than py_call, since not only tasks but also new data
objects may be introduced.

On the implementation level, HyperLoom contains a C++ library **libloom** that
implements the worker in an extensible way.

.. _Extending_new_tasks:

New tasks
---------

Let us assume that we want to implement a task that returns a number of a
specified characters in a D-object. First, we define the code of the task itself:

.. code-block:: c++

    #include "libloom/threadjob.h"

    class CountJob : public loom::ThreadJob
    {
    public:
       using ThreadJob::ThreadJob;

       std::shared_ptr<loom::Data> run() {
           // Verify inputs and configuration
           if (inputs.size() != 1 || task.config.size() != 1) {
                set_error("Invalid use of the task");
                return nullptr;
           }
           char c = task.config[0]; // Get first character of config

           if (!inputs[0].has_raw_data()) {
                set_error("Input object does not contain raw data");
                return nullptr;
           }

           // Get pointer to raw data
           const char *mem = inputs[0].get_raw_data();

           // Perform the computation
           size_t size = inputs[0].get_size();
           uint64_t count = 0;
           for (size_t i = 0; i < size;i ++) {
                if (mem[i] == c) {
                     count += 1;
                }
           }

           // Create result
           auto output = std::make_shared<RawData>();
           output->init_from_mem(work_dir, &count, sizeof(count));
           return std::static_pointer_cast<Data>(output);
       }
   };

``loom::ThreadJob`` serves for defining a tasks that are executed in its own
thread. The subclass has to implement ``run()`` method that is executed when the
task is fired. It should return data object or ``nullptr`` when an error occurs.

The following code defines ``main`` function for the modified worker. It is
actually the same code as for the worker distributed with HyperLoom except the
registartion of our new task. Each task has to be registered under a symbol.
Symbols for buildin tasks, data objects and resource requests starts with prefix
`loom/`. To avoid name clashes, it is good practice to introduce new prefix, in
our example, it is prefix `my/`.

.. code-block:: c++

    #include "libloom/worker.h"
    #include "libloom/log.h"
    #include "libloom/config.h"

    #include <memory>

    using namespace loom;

    int main(int argc, char **argv)
    {
        /* Create a configuration and parse args */
        Config config;
        config.parse_args(argc, argv);

        /* Init libuv */
        uv_loop_t loop;
        uv_loop_init(&loop);

        /* Create worker */
        loom::Worker worker(&loop, config);
        worker.register_basic_tasks();

        /* --> Registration of our task <-- */
        worker.add_task_factory<ThreadTaskInstance<CountJob>>("my/count");

        /* Start loop */
        uv_run(&loop, UV_RUN_DEFAULT);
        uv_loop_close(&loop);
        return 0;
    }


New data objects
----------------

**TODO**
