
from .task import Task, ResourceRequest
from .task import POLICY_SCHEDULER, POLICY_SIMPLE

import struct
from ..pb import loomrun_pb2 as loomrun
import cloudpickle


def cpus(value):
    r = ResourceRequest()
    r.add_resource("loom/resource/cpus", value)
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

ARRAY_MAKE = "loom/array/make"

RUN = "loom/run/run"

SCHEDULER_DSLICE = "loom/scheduler/dslice"
SCHEDULER_DGET = "loom/scheduler/dget"

PY_CALL = "loom/py/call"


u64 = struct.Struct("<Q")
u64u64 = struct.Struct("<QQ")


def dslice(input):
    task = Task()
    task.task_type = SCHEDULER_DSLICE
    task.policy = POLICY_SCHEDULER
    task.inputs = (input,)
    return task


def dget(input):
    task = Task()
    task.task_type = SCHEDULER_DGET
    task.policy = POLICY_SCHEDULER
    task.inputs = (input,)
    return task


def const(data):
    task = Task()
    task.task_type = DATA_CONST
    task.config = data
    task.policy = POLICY_SIMPLE
    return task


def merge(inputs, delimiter=""):
    assert isinstance(delimiter, str)
    task = Task()
    task.task_type = DATA_MERGE
    task.inputs = inputs
    task.policy = POLICY_SIMPLE
    if delimiter:
        task.config = delimiter
    return task


def open(filename):
    task = Task()
    task.task_type = DATA_OPEN
    task.config = filename
    task.policy = POLICY_SIMPLE
    return task


def split(input, char=None):
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
    task = Task()
    task.task_type = ARRAY_MAKE
    task.inputs = inputs
    task.policy = POLICY_SIMPLE
    return task


def size(input):
    task = Task()
    task.task_type = BASE_SIZE
    task.inputs = (input,)
    task.policy = POLICY_SIMPLE
    return task


def length(input):
    task = Task()
    task.task_type = BASE_LENGTH
    task.inputs = (input,)
    task.policy = POLICY_SIMPLE
    return task


def get(input, index):
    task = Task()
    task.task_type = BASE_GET
    task.inputs = (input,)
    task.config = u64.pack(index)
    task.policy = POLICY_SIMPLE
    return task


def slice(input, start, end):
    task = Task()
    task.task_type = BASE_SLICE
    task.inputs = (input,)
    task.config = u64u64.pack(start, end)
    task.policy = POLICY_SIMPLE
    return task


def py_call(obj, inputs=(), request=cpu1):
    task = Task()
    task.task_type = PY_CALL
    task.inputs = inputs
    task.config = cloudpickle.dumps(obj)
    return task


def py_task(label=None):
    def make_py_call(fn):
        def py_task_builder(*args):
            task = py_call(fn, args)
            if label is not None:
                task.label = label
            else:
                task.label = fn.__name__
            return task
        assert callable(fn)
        return py_task_builder
    return make_py_call
