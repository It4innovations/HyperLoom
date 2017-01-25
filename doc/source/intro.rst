
Introduction
============

**Loom** is a platform for defining and executing workflow pipelines in a
distributed environment. **Loom** is designed to provide a scalable framework
that is able to efficiently execute many small interconnected tasks on HPC (High
Performance Computing) infrastructure.

A user provides a description of the computation in a form of DAG (Directed
Acyclic Graph) that captures dependencies between tasks. The infrastructure
automatically schedules tasks on available nodes while managing all necessary
data transfers.


Architecture
------------

*Loom* architecture is depicted in :numref:`architecture`.
The main components are:

* **client** -- A gateway for user -- it serves for submitting a plan (in form
  of a DAG) to the server and waits for the results of the computation. Loom is distributed with 
  Python client.

* **server** -- The main process that orchestrates the execution of computation.
  It processes a plan received from the client and instructs the worker
  processes.

* **worker** -- A process that is controlled by the server and performs the
  actual computation. Loom provides ways to extend worker by user codes to
  provide new kind of tasks or data types. (Server and worker are written in
  C++)

.. figure:: arch.png
   :width: 400
   :alt: Architecture scheme
   :name: architecture

   Architecture of Loom


Basic terms
-----------

The basic elements of Loom's programming model are: **data object**, **task**,
and **plan**. A **data object** is an arbitrary data structure that can be
serialized/deserialized. A **task** represents a computations that produces data
objects and **plan** is a set of computations.


Data objects
++++++++++++

Data objects are fundamental entities in Loom. They represent values that serves
as arguments and results of tasks. There are the following build-in basic types
of data objects:

* **Plain object** -- An anonymous sequence of bytes without any additional
  interpretation by Loom.

* **File** -- A handler to an external file on shared file system. From the
  user's perspective, it behaves like a plain object; except when a data
  transfer between nodes occurs, only a path to the file is transferred.

* **Array** -- A sequence of arbitrary data objects

* **Index** -- A logical view over a D-Object data object with a list of positions.
  It is used to slice data according some positions (e.g. positions of the
  new-line character to extract lines). It behaves like an array without
  explicit storing of each entry.

* **PyObj** -- Contains an arbitrary Python object

We call objects that are able to provide a content as continous
chunk of memory as **D-Objects**. Plain object and File object are D-Objects;
Array, Index, and PyObj are *not* D-Objects.

Each data object

* **size** -- the number of bytes needed to store the object
* **length** -- the number of 'inner pieces'. Length is zero when an object has no
  inner structure. Plain objects and files have always zero length; an array has length
  equal to number of lements in the array.

.. Note:: **size** is an approximation. For a plain object, it is the length of
          data itself without any metada. The size of an array is a sum of sizes
          of elements. The size of PyObj is obtained by ``sys.getsizeof``.

Tasks
+++++

A **task** is an object representing a computation together with its
dependencies and a configuration. Each task has the following attributes:

* Task inputs -- task's prerequisites (some other tasks)
* Task type -- the specification of the procedure that should be executed
* Task policy -- defines how should be the task scheduled
* Configuration -- a sequence of bytes that is interpreted according the task type
* Resource constraints

By task *execution*, we mean executing a procedure according to *task type*,
which takes data objects and configuration, and returns a new data object. The
input data objects are obtained as a result of executing tasks defined in task
inputs. Resource constraints serve to express that a task execution may need
some specific hardware or number of processes.


Plan
++++

**Plan** is a set of tasks. Plan has to form a finite asyclic directed
multigraph where nodes are tasks and arcs express input dependencies between
tasks. *Plan execution* is an execution of tasks according to the dependencies
defined in the graph.

.. Note::

  * We have formally restricted each task to return only a single data object as
    its result. However, a task can produce more results by returning an array of
    data objects.
  * Input data objects are always results of a previous tasks. To create a
    specific constant data object, there is a standard task (``tasks.const`` in
    Python API) that takes no input and only creates a data object from its
    configuration.


Symbols
+++++++

Customization and extendability are important concepts of Loom. Loom is designed
to enable creating customized workers that providies new task types, data
objects and resources. Loom uses the concept of name spaces to avoid potential
name clashes between different workers. Each type of data object, task type and
resource type is identified by a symbol. Symbols are hierarchically organized
and the slash character `/` is used as the separator of each level (e.g.
`loom/data/const`). All built-in task types, data object types, and resource
types always start with `loom/` prefix. Other objects introduced in a a
specialized worker should introduce its own prefix.
