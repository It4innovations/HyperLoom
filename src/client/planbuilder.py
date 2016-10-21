
from .plan import Plan, Task, ResourceRequest
from .plan import POLICY_SCHEDULER, POLICY_SIMPLE

import struct
import loomrun_pb2


def cpus(value):
    r = ResourceRequest()
    r.add_resource("loom/resource/cpus", value)
    return r

cpu1 = cpus(1)


class PlanBuilder(object):

    TASK_BASE_GET = "loom/base/get"
    TASK_BASE_SLICE = "loom/base/slice"
    TASK_BASE_SIZE = "loom/base/size"
    TASK_BASE_LENGTH = "loom/base/length"

    TASK_DATA_CONST = "loom/data/const"
    TASK_DATA_MERGE = "loom/data/merge"
    TASK_DATA_OPEN = "loom/data/open"
    TASK_DATA_SPLIT = "loom/data/split"

    TASK_ARRAY_MAKE = "loom/array/make"

    TASK_RUN = "loom/run/run"

    TASK_SCHEDULER_DSLICE = "loom/scheduler/dslice"
    TASK_SCHEDULER_DGET = "loom/scheduler/dget"

    u64 = struct.Struct("<Q")
    u64u64 = struct.Struct("<QQ")

    def __init__(self, plan=None):
        if plan is None:
            plan = Plan()
        self.plan = plan

    def task_dslice(self, input):
        task = Task()
        task.task_type = self.TASK_SCHEDULER_DSLICE
        task.policy = POLICY_SCHEDULER
        task.inputs = (input,)
        return self.plan.add(task)

    def task_dget(self, input):
        task = Task()
        task.task_type = self.TASK_SCHEDULER_DGET
        task.policy = POLICY_SCHEDULER
        task.inputs = (input,)
        return self.plan.add(task)

    def task_const(self, data):
        task = Task()
        task.task_type = self.TASK_DATA_CONST
        task.config = data
        task.policy = POLICY_SIMPLE
        return self.plan.add(task)

    def task_merge(self, inputs):
        task = Task()
        task.task_type = self.TASK_DATA_MERGE
        task.inputs = inputs
        task.policy = POLICY_SIMPLE
        return self.plan.add(task)

    def task_open(self, filename):
        task = Task()
        task.task_type = self.TASK_DATA_OPEN
        task.config = filename
        task.policy = POLICY_SIMPLE
        return self.plan.add(task)

    def task_split(self, input, char=None):
        task = Task()
        task.task_type = self.TASK_DATA_SPLIT
        task.inputs = (input,)
        task.policy = POLICY_SIMPLE
        return self.plan.add(task)

    def task_run(self,
                 args,
                 inputs=(),
                 outputs=(None,),
                 stdin=None,
                 request=cpu1):

        if isinstance(args, str):
            args = args.split()

        if stdin is not None:
            inputs = ((stdin, None),) + tuple(inputs)

        task = Task()
        task.task_type = self.TASK_RUN
        task.inputs = tuple(i for i, fname in inputs)

        msg = loomrun_pb2.Run()
        msg.args.extend(args)

        msg.map_inputs.extend(fname if fname else "+in"
                              for i, fname in inputs)
        msg.map_outputs.extend(fname if fname else "+out"
                               for fname in outputs)
        task.config = msg.SerializeToString()
        task.resource_request = request
        return self.plan.add(task)

    def task_array_make(self, inputs):
        task = Task()
        task.task_type = self.TASK_ARRAY_MAKE
        task.inputs = inputs
        task.policy = POLICY_SIMPLE
        return self.plan.add(task)

    def task_size(self, input):
        task = Task()
        task.task_type = self.TASK_BASE_SIZE
        task.inputs = (input,)
        task.policy = POLICY_SIMPLE
        return self.plan.add(task)

    def task_length(self, input):
        task = Task()
        task.task_type = self.TASK_BASE_LENGTH
        task.inputs = (input,)
        task.policy = POLICY_SIMPLE
        return self.plan.add(task)

    def task_get(self, input, index):
        task = Task()
        task.task_type = self.TASK_BASE_GET
        task.inputs = (input,)
        task.config = self.u64.pack(index)
        task.policy = POLICY_SIMPLE
        return self.plan.add(task)

    def task_slice(self, input, start, end):
        task = Task()
        task.task_type = self.TASK_BASE_SLICE
        task.inputs = (input,)
        task.config = self.u64u64.pack(start, end)
        task.policy = POLICY_SIMPLE
        return self.plan.add(task)
