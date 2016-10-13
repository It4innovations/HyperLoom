
import loomplan_pb2
import loomrun_pb2

import struct


POLICY_STANDARD = loomplan_pb2.Task.POLICY_STANDARD
POLICY_SIMPLE = loomplan_pb2.Task.POLICY_SIMPLE
POLICY_SCHEDULER = loomplan_pb2.Task.POLICY_SCHEDULER


class Task(object):

    inputs = ()
    id = None
    config = ""
    policy = POLICY_STANDARD
    resource_request = None

    def set_message(self, msg, symbols, requests):
        msg.config = self.config
        msg.task_type = symbols[self.task_type]
        msg.input_ids.extend(t.id for t in self.inputs)
        msg.policy = self.policy
        if self.resource_request:
            msg.resource_request_index = requests.index(self.resource_request)


class ResourceRequest(object):

    def __init__(self):
        self.resources = {}

    def add_resource(self, name, value):
        self.resources[name] = value

    @property
    def names(self):
        return self.resources.keys()

    def set_message(self, msg, symbols):
        for name, value in self.resources.items():
            r = msg.resources.add()
            r.resource_type = symbols[name]
            r.value = value

    def __eq__(self, other):
        if not isinstance(other, ResourceRequest):
            return False
        return self.resources == other.resources

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash(tuple(sorted(self.resources.items())))


def cpus(value):
    r = ResourceRequest()
    r.add_resource("loom/resource/cpus", value)
    return r

cpu1 = cpus(1)


class Plan(object):

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

    def __init__(self):
        self.tasks = []

    def add(self, task):
        assert task.id is None
        task.id = len(self.tasks)
        self.tasks.append(task)
        return task

    def task_dslice(self, input):
        task = Task()
        task.task_type = self.TASK_SCHEDULER_DSLICE
        task.policy = POLICY_SCHEDULER
        task.inputs = (input,)
        return self.add(task)

    def task_dget(self, input):
        task = Task()
        task.task_type = self.TASK_SCHEDULER_DGET
        task.policy = POLICY_SCHEDULER
        task.inputs = (input,)
        return self.add(task)

    def task_const(self, data):
        task = Task()
        task.task_type = self.TASK_DATA_CONST
        task.config = data
        task.policy = POLICY_SIMPLE
        return self.add(task)

    def task_merge(self, inputs):
        task = Task()
        task.task_type = self.TASK_DATA_MERGE
        task.inputs = inputs
        task.policy = POLICY_SIMPLE
        return self.add(task)

    def task_open(self, filename):
        task = Task()
        task.task_type = self.TASK_DATA_OPEN
        task.config = filename
        task.policy = POLICY_SIMPLE
        return self.add(task)

    def task_split(self, input, char=None):
        task = Task()
        task.task_type = self.TASK_DATA_SPLIT
        task.inputs = (input,)
        task.policy = POLICY_SIMPLE
        return self.add(task)

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
        return self.add(task)

    def task_array_make(self, inputs):
        task = Task()
        task.task_type = self.TASK_ARRAY_MAKE
        task.inputs = inputs
        task.policy = POLICY_SIMPLE
        return self.add(task)

    def task_size(self, input):
        task = Task()
        task.task_type = self.TASK_BASE_SIZE
        task.inputs = (input,)
        task.policy = POLICY_SIMPLE
        return self.add(task)

    def task_length(self, input):
        task = Task()
        task.task_type = self.TASK_BASE_LENGTH
        task.inputs = (input,)
        task.policy = POLICY_SIMPLE
        return self.add(task)

    def task_get(self, input, index):
        task = Task()
        task.task_type = self.TASK_BASE_GET
        task.inputs = (input,)
        task.config = self.u64.pack(index)
        task.policy = POLICY_SIMPLE
        return self.add(task)

    def task_slice(self, input, start, end):
        task = Task()
        task.task_type = self.TASK_BASE_SLICE
        task.inputs = (input,)
        task.config = self.u64u64.pack(start, end)
        task.policy = POLICY_SIMPLE
        return self.add(task)

    def set_message(self, msg, symbols):
        requests = set()
        for task in self.tasks:
            if task.resource_request:
                requests.add(task.resource_request)
        requests = list(requests)

        for request in requests:
            r = msg.resource_requests.add()
            request.set_message(r, symbols)

        for task in self.tasks:
            t = msg.tasks.add()
            task.set_message(t, symbols, requests)
        return msg
