
from .task import Task, ResourceRequest
from .task import POLICY_SCHEDULER, POLICY_SIMPLE

import struct
from ..pb import loomrun_pb2 as loomrun
import cloudpickle


def cpus(n):
    """Returns resource requests that asks for ``n`` cpus

    Args:
        n (int): Number of cpus

    Returns:
        ResourceRequest

    """
    r = ResourceRequest()
    r.add_resource("loom/resource/cpus", n)
    return r

cpu1 = cpus(1)


BASE_GET = "loom/base/get"
BASE_SLICE = "loom/base/slice"
BASE_SIZE = "loom/base/size"
BASE_LENGTH = "loom/base/length"

DATA_CONST = "loom/data/const"
DATA_MERGE = "loom/data/merge"
DATA_OPEN = "loom/data/open"
DATA_SPLIT = "loom/data/split"
DATA_SAVE = "loom/data/save"

ARRAY_MAKE = "loom/array/make"

RUN = "loom/run/run"

SCHEDULER_DSLICE = "loom/scheduler/dslice"
SCHEDULER_DGET = "loom/scheduler/dget"

PY_CALL = "loom/py/call"
PY_VALUE = "loom/py/value"


u64 = struct.Struct("<Q")
u64u64 = struct.Struct("<QQ")


def dslice(input):
    """Dynamic 'slice'

    Decomposes the input on several slices; the number of slices is chosen
    dynamically by the scheduler to produce the optimal number of tasks.

    Note:
        This is **scheduler task**, that dynamically transforms the task graph

    Examples:
        >>> t1 = tasks.dslice(x)
        >>> t2 = tasks.XXX(..., t1, ...)
        >>> result = tasks.array_make((t2,))

    is roughly equivalent to the following code:

        >>> t1 = tasks.slice(x, 0, N1)
        >>> s1 = tasks.XXX(..., t1, ...)
        >>> t2 = tasks.slice(x, N1, N2)
        >>> s2 = tasks.XXX(..., t2, ...)
        ...
        >>> tk = tasks.slice(x, Nk-1, Nk)
        >>> sk = tasks.XXX(..., tk, ...)
        >>> result = tasks.array_make((s1, ..., sk))
    """
    task = Task()
    task.task_type = SCHEDULER_DSLICE
    task.policy = POLICY_SCHEDULER
    task.inputs = (input,)
    return task


def dget(input):
    """Dynamic 'get'

    Apply an operation that follows this tasks on each element; the number of
    tasks depends on the length of data object. If the length is zero, then
    an error is reported.

    Note:
        This is **scheduler task**, that dynamically transforms the task graph

    Examples:
        >>> t1 = tasks.dget(x)
        >>> t2 = tasks.XXX(..., t2, ...)
        >>> result = tasks.make_array((t2,))

    is roughly equivalent to the following code (where is 'N' is length of the
    data object produced by ``x``.

        >>> t1 = tasks.get(x, 0)
        >>> s1 = tasks.XXX(..., t1, ...)
        >>> t2 = tasks.get(x, 1)
        >>> s2 = tasks.XXX(..., t2, ...)
        ...
        >>> tN = tasks.get(x, N)
        >>> sN = tasks.XXX(..., tk, ...)
        >>> result = tasks.array_make((s1, ..., sN))

    """

    task = Task()
    task.task_type = SCHEDULER_DGET
    task.policy = POLICY_SCHEDULER
    task.inputs = (input,)
    return task


def const(data):
    """Task that creates a new plain data object

    Args:
        data (str or bytes): A content of the object

    Returns:
        Task

    Example:
        >>> t = tasks.const("Hello")
        >>> client.submit(t)
        b'Hello'
    """

    task = Task()
    task.task_type = DATA_CONST
    task.config = data
    task.policy = POLICY_SIMPLE
    return task


def merge(inputs, delimiter=""):
    """Creates a task that concatenates a D-objects into one plain object

    Args:
        inputs (sequence of tasks): A sequence of tasks that should produce
                                    a raw data objects -- these objects are
                                    concatenated.
        delimiter (str): A sequence that is put between two consecutive
                         data objects.

    Returns:
        Task
    """

    assert isinstance(delimiter, str)
    task = Task()
    task.task_type = DATA_MERGE
    task.inputs = inputs
    task.policy = POLICY_SIMPLE
    if delimiter:
        task.config = delimiter
    return task


def open(filename):
    """Create a task that creates a file object.

    Note: It just opens a file (without reading), hence the operation itself is
    fast.

    Args:
       filename (str): Path to file that is opened

    Returns:
        Task

    """

    task = Task()
    task.task_type = DATA_OPEN
    task.config = filename
    task.policy = POLICY_SIMPLE
    return task


def split(input, char=None):
    """Task that creates an index by splitting a D-object by a delimiter

    Args:
        char (str): Delimiter (if None than 'new line' is used)

    Returns:
        Task
    """
    task = Task()
    task.task_type = DATA_SPLIT
    task.inputs = (input,)
    task.policy = POLICY_SIMPLE
    return task


def run(args,
        inputs=(),
        outputs=(None,),
        stdin=None,
        request=cpu1):
    """Task that runs an external program

    Args:
        args (sequence or str): Arguments of the executed program.
                                The first argument is the name of the program.
                                If ``args`` is ``str``
                                then ``.split()`` method is applied on the
                                the string to get arguments.
        inputs (sequence):
            Sequence of pairs (task, target).
            Task has to be a D-object.
            If target is ``None`` then
            ``task`` is mapped on stdin, otherwise ``target`` must be string.
            If target starts with '$' then ``task`` is mapped into a variable
            that can be used in ``args``, otherwise the ``target`` is
            interpreted as filename.
        outputs (sequence):
            Sequence of filenames from which the output object
            is composed. If filename is ``None`` then stdout is used.
            If ``len(outputs) == 1`` then single plain object is result of
            the task otherwise an array of plain objects is the result.
        stdin (task or None):
            If ``stdin`` is ``Task`` than it is mapped to stdin.

    Returns:
        Task
    """

    if isinstance(args, str):
        args = args.split()

    if stdin is not None:
        inputs = ((stdin, None),) + tuple(inputs)

    task = Task()
    task.task_type = RUN
    task.inputs = tuple(i for i, fname in inputs)

    msg = loomrun.Run()
    msg.args.extend(args)

    msg.map_inputs.extend(fname if fname else "+in"
                          for i, fname in inputs)
    msg.map_outputs.extend(fname if fname else "+out"
                           for fname in outputs)
    task.config = msg.SerializeToString()
    task.resource_request = request
    return task


def array_make(inputs):
    """Creates a task that creates an array from inputs

    Args:
        inputs (sequence of tasks): Tasks that are computed to get
                                    elements of the array

    Returns:
        Task

    Example:
       >>> a = tasks.const("abc")
       >>> b = tasks.const("123")
       >>> c = tasks.array_make((a, b))
       >>> client.submit(c)
       [b'abc', b'123']
    """

    task = Task()
    task.task_type = ARRAY_MAKE
    task.inputs = inputs
    task.policy = POLICY_SIMPLE
    return task


def size(input):
    """Creates a task that takes one object and returns its size"""
    task = Task()
    task.task_type = BASE_SIZE
    task.inputs = (input,)
    task.policy = POLICY_SIMPLE
    return task


def length(input):
    """Creates a task that takes one object and returns its size"""
    task = Task()
    task.task_type = BASE_LENGTH
    task.inputs = (input,)
    task.policy = POLICY_SIMPLE
    return task


def get(input, index):
    """Creates a task that gets a sub-object for a given position

    Args:
       input (Task): Task with an internal structure
       index (int): A value within a range 0 .. length - 1 of the input.
                    If the index is out of range, the task fails

    Returns:
       Task
    """
    task = Task()
    task.task_type = BASE_GET
    task.inputs = (input,)
    task.config = u64.pack(index)
    task.policy = POLICY_SIMPLE
    return task


def slice(input, start, end):
    """Creates a task that takes a slice from a given data object.

    Args:
       input (Task): Task with an internal structure
       start (int): A value within a range 0 .. length - 1 of the input.
       end (int): A value within a range 0 .. length of the input.

    Returns:
       Task
    """

    task = Task()
    task.task_type = BASE_SLICE
    task.inputs = (input,)
    task.config = u64u64.pack(start, end)
    task.policy = POLICY_SIMPLE
    return task


def save(input, filename):
    """Save D-object into file.

    Args:
       input (Task): D-object
       filename (str): An absolute filename where D-object is saved.

    Returns:
       Empty data object
    """

    task = Task()
    task.task_type = DATA_SAVE
    task.inputs = (input,)
    task.config = filename
    task.resource_request = cpu1
    return task


def py_call(obj, inputs=(), direct_args=()):
    """Create a task that calls Python code

    Example:

    >>> def hello(x):
           return b"Hello " + x.read()
    >>> a = tasks.const("Loom")
    >>> b = tasks.py_call((a,), hello)
    >>> client.submit(b)
    b'Hello Loom'
    """

    task = Task()
    task.task_type = PY_CALL
    task.inputs = (obj,) + tuple(inputs)
    task.config = cloudpickle.dumps(tuple(direct_args))
    task.resource_request = cpu1
    return task


def py_value(obj):
    """Task that creates a constant Python value"""

    task = Task()
    task.task_type = PY_VALUE
    task.config = cloudpickle.dumps(obj)
    task.resource_request = cpu1
    return task


def py_task(label=None, request=cpu1, context=False, n_direct_args=0):
    """Decorator that creates a task builder form a function

    Example:

    >>> @tasks.py_task()
        def hello(x):
           return b"Hello " + x.read()
    >>> a = tasks.const("Loom")
    >>> b = hello(a)
    >>> client.submit(b)
    b'Hello Loom'
    """

    def make_py_call(fn):
        fn_obj = py_value((fn, bool(context)))
        fn_obj.label = "_loom:" + fn.__name__

        def py_task_builder(*args):
            inputs = args[n_direct_args:]
            direct_args = args[:n_direct_args]
            task = py_call(fn_obj, inputs, direct_args)
            task.resource_request = request
            if label is not None:
                task.label = label
            else:
                task.label = fn.__name__
            return task
        assert callable(fn)
        return py_task_builder
    return make_py_call
