
Python client
=============

Basic usage
-----------

The following code contains a simple example of Loom usage. It creates two
constants and a task that merge them. Next, it creates a client and connect to
the server and submits the plan. When the computation is finished, the
``submit`` method returns with the result. It assumes that the server is running
at address *localhost* on TCP port 9010.

.. code-block:: python

  from loom.client import Client, tasks

  task1 = tasks.const("Hello ")        # Create a plain object
  task2 = tasks.const("world!")        # Create a plain object
  task3 = tasks.merge((task1, task2))  # Merge two data objects together

  client = Client("localhost", 9010)   # Create a client
  client.submit(task3)                 # Submit task; it returns b"Hello world!"


Submiting more tasks
++++++++++++++++++++

It is possible to submit more tasks at once. When the first argument of
``submit`` is a list of tasks, then ``submit`` waits until all tasks are not
completed.

.. code-block:: python

  from loom.client import Client, tasks

  ts = [tasks.const(str(x) for x in range(100)]
  client = Client("localhost", 9010)
  client.submit(ts)  # returns [b"0", b"1", b"2", ... , b"99"]

The full list of all tasks can be found in :ref:`PyClient_API_Tasks`.


Running external programs
-------------------------

In this subsection, we demonstrate a running of external programs. The most
basic scenario is execution of a program while mapping a data object on standard
input and capturing the standard output. It can be achieved by the following
code:

.. code-block:: python

   task1 = ...
   task_run = tasks.run("/bin/grep Loom", stdin=task1)

If the ``task_run`` is executed, the standard unix program `grep` is executed.
Result from ``task`` is mapped on its standard input and output is captured.
Therefore, this example creates a new plain data object that contains only lines
containing string `Loom`.

If the first argument is string, as in the above example, then Loom expects that
arguments are separated by white spaces. But argument may be provided
explicitly, e.g.

.. code-block:: python

   task_run = tasks.run(("/path/to/program", "--arg1", "argument with spaces"))


Mapping input files
+++++++++++++++++++

If the executed program cannot read data from the standard input or we need to
provide more inputs, ``run`` allows to map data objects to files.

The following code maps the result of ``task_a`` to `file1` and result of
``task_b`` to `file2`.

.. code-block:: python

   task_a = ...
   task_b = ...
   task_run = tasks.run("/bin/cat file1 file2",
                        [(task_a, "file1"), (task_b, "file2")])

A new fresh directory is created for each execution of the program and the
current working directory is set to this directory. Files created by mapping
data objects are placed to this directory. Therefore, as far as only relative
paths are used, no file conflict occurs. Therefore the following code is
correct, even all three tasks may be executed on the same node
simultaneously.

.. code-block:: python

   task_a = ...
   task_b = ...
   task_c = ...

   task_1 = tasks.run("/bin/cat file1", [(task_a, "file1")])
   task_2 = tasks.run("/bin/cat file1", [(task_b, "file1")])
   task_3 = tasks.run("/bin/cat file1", [(task_c, "file1")])


Mapping output files
++++++++++++++++++++

So far, the result of ``run`` tasks is created by gathering the standard output.
There is also an option to create a result from files created by the program
execution.

Let us assume that program `/path/program1` creates `outputs.txt` as the output,
then we can run the following program and capturing the file at the end
(standard output of the program is ignored).

.. code-block:: python

   task = tasks.run("/path/program1", outputs=("output.txt",))

The user may define more files as the output. Let us consider the following
code, that assumes that `program2` creates two files.

.. code-block:: python

   task = tasks.run("/path/program2", outputs=("output1.txt", "output2.txt"))

The result of this task is an array with two elements. This array contains with
two plain data objects.

If ``None`` is used instead of a name of a file, than the standard output is
captured. Therefore, the following task creates a three element array:


.. code-block:: python

   task = tasks.run("/path/program3",
                    outputs=("output1.txt",  # 1st element of array is got from 'output1.txt'
                             None,           # 2nd element of array is stdout
                             "output2.txt")) # 3rd element of array is got from 'output2.txt'


Variables
+++++++++

In previous examples, we have always used a constant arguments for programs;
however, programs arguments can be also parametrized by data objects. When an
input data object is mapped to a file name that starts with character `$` then
no file is mapped, but the variable with the same name can be used in
arguments. Loom expands the variable before the execution of the task.

The following example executes program `ls` where the first argument is
obtained from data object.

.. code-block:: python

   path = tasks.const("/some/path")
   task = tasks.run("/bin/ls $PATH", [(path, "$PATH")])

.. note::

    See :ref:`PyClient_redirects` for a more powerfull dynamic configuration of
    ``run``.


Error handling
++++++++++++++

When an executed program exits with a non-zero exit code then the server reports
an error that is propagated as ``TaskFailed`` exception in the client.

.. code-block:: python

   task = tasks.run("ls /non-existent-path")
   try:
      client.submit(task)
   except TaskFailed as e:
      print("Error: " + str(e))

This program prints the following:

.. code-block:: text

   Error: Task id=2 failed: Program terminated with status 2
   Stderr:
   ls: cannot access '/non-existing-dictionary': No such file or directory


.. _PyClient_pytasks:

Python functions in plans
-------------------------

Loom allows to execute directly python functions as tasks. The easiest way is to
use decorator ``py_task()``. This is demonstrated by the following code: ::

    from loom.client import tasks

    @tasks.py_task()
    def hello(a):
        return b"Hello " + a.read()

    task1 = tasks.cont("world")
    task2 = hello(task1)

    client.submit(task2)  # returns b"Hello world"

The ``hello`` function is seralized and sent to the server. The server executes
the function on a worker that has necessary data.

  * When ``str`` or ``bytes`` is returned from the function then a new plain
    data object is created.
  * When ``loom.client.Task`` is returned then the the task redirection is
    used (see :ref:`PyClient_redirects`).
  * When something else is returned or exeption is thrown then the task fails.
  * Input arguments are wrapped by objects that provide the following methods

     * ``read()`` - returns the content of the object as ``bytes``, if data
       object is not D-Object than empty bytes are returned.
     * ``size()`` - returns the size of the data object
     * ``length()`` - returns the length of the data object

  * ``tasks.py_task`` has optional ``label`` parameter to set a label of the
    task if it is not used, then the name of the function is used. See XXX for
    more information about labels

Decorator ``py_task()`` actually uses :py:func:`loom.client.tasks.py_call`,
hence the code above can be written also as: ::

    from loom.client import tasks

    def hello(a):
        return b"Hello " + a.read()

    task1 = tasks.cont("world")
    task2 = tasks.py_call(hello, (task1,))
    task2.label = "hello"

    client.submit(task2)  # returns b"Hello world"


.. _PyClient_redirects:

Task redirection
----------------

Python tasks (used via decorator ``py_task`` or directoly via ``py_call``) may
return ``loom.client.Task`` to achive a task redirection. It is useful for
simple dynamic configuration of the plan.

Let us assume that we want to run ``tasks.run``, but configure it dynamically on
the actual data. The following function takes two arguments, checks the size and
then executes ``tasks.run`` with the bigger one::

    from loom.client import tasks

    @tasks.py_task()
    def my_run(a, b):
        if a.size() > b.size():
            data = a
        else:
            data = b
    return tasks.run("/some/program", stdin=data)


Task context
------------

Python task can configured to obtain a ``Context`` object as the first argument.
It provides interface for interacting with the Loom worker.
The following example demonstrates logging through context object::

    from loom.client import tasks

    @tasks.py_task(context=True)
    def hello(ctx, a):
        ctx.log_info("Hello was called")
        return b"Hello " + a.read()

The function is has the same behavior as the ``hello`` function in
:ref:`PyClient_pytasks`. But not it writes a message into the worker log.
``Context`` has five logging methods: ``log_debug``, ``log_info``, ``log_warn``,
``log_error``, and ``log_critical``.

Moreover ``Context`` has attribute ``task_id`` that holds the indentification
number of the task.


Reports
-------

Reporting system serves for debugging and profiling the Loom programs.
Report can be created by ``submit`` method by the following way::

   task = ...
   client.submit(task, "myreport.report")

It creates report file `myreport.report`. This file can be explored by ``loom.rview``.

The following command shows graph of the plan:

::

   $ python3 -m loom.rview myreport.report --show-graph

The following command shows tracelog of tasks (how long and where each tasks was
executed):

::

   $ python3 -m loom.rview myreport.report --show-trace

The full list of commands can be obtained by

::

   $ python3 -m loom.rview --help

In the example above, we have created report by ``submit``. It is possible to
create a report without submitting a task by
:py:func:`loom.client.make_dry_report`. The report created by this way contains
a graph of tasks, but does not contain runtime information (since the task was
not executed)::

   from loom.client import make_dry_report
   task = ...
   make_dry_report(task, "myreport.report")

Labels
------

Each task may optinally define a **label**. It serves for debugging purpose --
it changes how is the task shown in `rview`. Label has no influence on the
program execution. The label is defined as follows::

    task = tasks.const("Hello")
    task.label = "Initial data"

`rview` assigns colors of graph nodes or lines in a trace according the labels.
The two labels have the same color if they have the same prefix upto the first
occurence of character ``:``. In the following example, three colors will be
used. Tasks ``task1`` and ``task2`` will share the same color and ``task3`` and
``task4`` will also share the same color.

::

    task1.label = "Init"
    task2.label = "Init"
    task3.label = "Compute: 1"
    task4.label = "Compute: 2"
    task5.label = "End"


Resource requests
-----------------

Resource requests serves to specify some hardware limitations or inner
paralelism of tasks. The current version supports only requests for a number of
cores. It can be express as follows::

   from loom.client import tasks

   t1 = tasks.run("/a/parallel/program")
   t1.resource_request = tasks.cpus(4)

In this example, ``t1`` is a task that reserves 4 cpu cores. It means that if a
worker has 8 cores, that at most two of such tasks is executed simultaneously.
Note that if a worker has 3 or less cores, than ``t1`` is never scheduled on
such a worker.

When a task has no ``resource_request`` than scheduler assumes that the task is
a light weight one and it is executed very fast without resource demands (e.g.
picking an element from array). The scheduler is allows to schedule
simultenously more light weight tasks than cores available for the worker.

.. Important::
   Basic tasks defined module ``loom.tasks`` do not define any resource request;
   except ``loom.tasks.run``, ``loom.tasks.py_call``, and ``loom.tasks.py_task``
   by default defines resource request for 1 cpu core.


Dynamic slice & get
-------------------

Loom scheduler recognizes two special tasks that dynamically modify the plan --
**dynamic slice** and **dynamic get**. They dynamically create new tasks
according the length of a data object and the current number of workers and
their resources. The goal is to obtain an optimal number of tasks to utilize the
cluster resources.

The following example::

    t1 = tasks.dslice(x)
    t2 = tasks.XXX(..., t1, ...)
    result = tasks.array_make((t2,))

is roughly equivalent to the following code::

    t1 = tasks.slice(x, 0, N1)
    s1 = tasks.XXX(..., t1, ...)
    t2 = tasks.slice(x, N1, N2)
    s2 = tasks.XXX(..., t2, ...)
    ...
    tk = tasks.slice(x, Nk-1, Nk)
    sk = tasks.XXX(..., tk, ...)
    result = tasks.array_make((s1, ..., sk))

where 0 < N1 < N2 ... Nk where Nk is the length of the data object produced by
``x``.

Analogously, the following code:

     t1 = tasks.dget(x)
     t2 = tasks.XXX(..., t2, ...)
     result = tasks.make_array((t2,))

is roughly equivalent to the following code (where is *N* is the length of the
the data object produced by ``x``::

     t1 = tasks.get(x, 0)
     s1 = tasks.XXX(..., t1, ...)
     t2 = tasks.get(x, 1)
     s2 = tasks.XXX(..., t2, ...)
     ...
     tN = tasks.get(x, N)
     sN = tasks.XXX(..., tk, ...)
     result = tasks.array_make((s1, ..., sN))


Own tasks
---------

Module ``tasks`` contains tasks provided by the worker distributed with Loom. If
we extend a worker by our own special tasks, we also need a way how to call them
from the client.

Let us assume that we have extended the worker by task `my/count` as is shown in
:ref:`Extending_new_tasks`. We can create the following code to utilize this new
task type::

    from loom.client import Task, tasks

    def my_count(input, character):
        task = Task()
        task.task_type = "my/count"
        task.inputs = (input,)
        task.config = character
        return task

    t1 = tasks.open("/my/file")
    t2 = my_count(t1)

    ...

    client.submit(t2)
