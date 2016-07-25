
import loomplan_pb2
import loomrun_pb2
import gv

import struct


class Task(object):

    inputs = ()
    id = None
    config = ""

    """
    def __init__(self,  config, inputs=()):
        self.id = None
        self.task_type = task_type
        self.config = config
        self.inputs = inputs
    """

    def set_message(self, msg, task_types):
        msg.config = self.config
        msg.task_type = task_types.index(self.task_type)
        msg.input_ids.extend(t.id for t in self.inputs)


class Plan(object):

    TASK_BASE_GET = "base/get"
    TASK_BASE_SLICE = "base/slice"

    TASK_DATA_CONST = "data/const"
    TASK_DATA_MERGE = "data/merge"
    TASK_DATA_OPEN = "data/open"
    TASK_DATA_SPLIT_LINES = "data/split_lines"

    TASK_ARRAY_MAKE = "array/make"

    TASK_RUN = "run/run"

    TASK_SCHEDULER_DSLICE = "scheduler/dslice"

    u64 = struct.Struct("<Q")
    u64u64 = struct.Struct("<QQ")

    def __init__(self):
        self.tasks = []
        self.task_types = set()

    def add(self, task):
        assert task.id is None
        task.id = len(self.tasks)
        self.tasks.append(task)
        self.task_types.add(task.task_type)
        return task

    def task_dslice(self, input):
        task = Task()
        task.task_type = self.TASK_SCHEDULER_DSLICE
        task.inputs = (input,)
        return self.add(task)

    def task_const(self, data):
        task = Task()
        task.task_type = self.TASK_DATA_CONST
        task.config = data
        return self.add(task)

    def task_merge(self, inputs):
        task = Task()
        task.task_type = self.TASK_DATA_MERGE
        task.inputs = inputs
        return self.add(task)

    def task_open(self, filename):
        task = Task()
        task.task_type = self.TASK_DATA_OPEN
        task.config = filename
        return self.add(task)

    def task_split_lines(self, input, start, end):
        task = Task()
        task.task_type = self.TASK_DATA_SPLIT_LINES
        task.config = self.u64u64.pack(start, end)
        task.inputs = (input,)
        return self.add(task)

    def task_run(self, args, inputs=(), outputs=(None,), stdin=None):
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
        return self.add(task)

    def task_array_make(self, inputs):
        task = Task()
        task.task_type = self.TASK_ARRAY_MAKE
        task.inputs = inputs
        return self.add(task)

    def task_get(self, input, index):
        task = Task()
        task.task_type = self.TASK_BASE_GET
        task.inputs = (input,)
        task.config = self.u64.pack(index)
        return self.add(task)

    def task_slice(self, input, start, end):
        task = Task()
        task.task_type = self.TASK_BASE_SLICE
        task.inputs = (input,)
        task.config = self.u64u64.pack(start, end)
        return self.add(task)

    def create_message(self):
        task_types = list(self.task_types)
        task_types.sort()

        msg = loomplan_pb2.Plan()
        msg.task_types.extend(task_types)

        for task in self.tasks:
            t = msg.tasks.add()
            task.set_message(t, task_types)
        return msg

    def write_dot(self, filename, info=None):
        colors = ["red", "green", "blue", "orange", "violet"]
        if info:
            w = sorted(set(worker for id, worker in info))
            workers = {}
            for id, worker in info:
                workers[id] = w.index(worker)
            del w
        else:
            workers = None
        graph = gv.Graph()
        for task in self.tasks:
            node = graph.node(task.id)
            if workers:
                node.color = colors[workers[task.id] % len(colors)]
            node.label = "{}\n{}".format(str(task.id), task.task_type)
            for inp in task.inputs:
                graph.node(inp.id).add_arc(node)
        graph.write(filename)
