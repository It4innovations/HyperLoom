
import loomplan_pb2
import loomrun_pb2
import gv

import struct


MODE_STANDARD = loomplan_pb2.Task.MODE_STANDARD
MODE_SIMPLE = loomplan_pb2.Task.MODE_SIMPLE
MODE_SCHEDULER = loomplan_pb2.Task.MODE_SCHEDULER


class Task(object):

    inputs = ()
    id = None
    config = ""
    mode = MODE_STANDARD

    def set_message(self, msg, symbols):
        msg.config = self.config
        msg.task_type = symbols[self.task_type]
        msg.input_ids.extend(t.id for t in self.inputs)
        msg.mode = self.mode


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
        task.mode = MODE_SCHEDULER
        task.inputs = (input,)
        return self.add(task)

    def task_dget(self, input):
        task = Task()
        task.task_type = self.TASK_SCHEDULER_DGET
        task.mode = MODE_SCHEDULER
        task.inputs = (input,)
        return self.add(task)

    def task_const(self, data):
        task = Task()
        task.task_type = self.TASK_DATA_CONST
        task.config = data
        task.mode = MODE_SIMPLE
        return self.add(task)

    def task_merge(self, inputs):
        task = Task()
        task.task_type = self.TASK_DATA_MERGE
        task.inputs = inputs
        task.mode = MODE_SIMPLE
        return self.add(task)

    def task_open(self, filename):
        task = Task()
        task.task_type = self.TASK_DATA_OPEN
        task.config = filename
        task.mode = MODE_SIMPLE
        return self.add(task)

    def task_split(self, input, char=None):
        task = Task()
        task.task_type = self.TASK_DATA_SPLIT
        task.inputs = (input,)
        task.mode = MODE_SIMPLE
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
        task.mode = MODE_SIMPLE
        return self.add(task)

    def task_size(self, input):
        task = Task()
        task.task_type = self.TASK_BASE_SIZE
        task.inputs = (input,)
        task.mode = MODE_SIMPLE
        return self.add(task)

    def task_length(self, input):
        task = Task()
        task.task_type = self.TASK_BASE_LENGTH
        task.inputs = (input,)
        task.mode = MODE_SIMPLE
        return self.add(task)

    def task_get(self, input, index):
        task = Task()
        task.task_type = self.TASK_BASE_GET
        task.inputs = (input,)
        task.config = self.u64.pack(index)
        task.mode = MODE_SIMPLE
        return self.add(task)

    def task_slice(self, input, start, end):
        task = Task()
        task.task_type = self.TASK_BASE_SLICE
        task.inputs = (input,)
        task.config = self.u64u64.pack(start, end)
        task.mode = MODE_SIMPLE
        return self.add(task)

    def create_message(self, symbols):
        msg = loomplan_pb2.Plan()
        for task in self.tasks:
            t = msg.tasks.add()
            task.set_message(t, symbols)
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
