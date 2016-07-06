
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


class ConstTask(Task):

    task_type = "const"

    def __init__(self, data):
        self.config = data


class MergeTask(Task):

    task_type = "merge"

    def __init__(self, inputs):
        self.inputs = inputs


class OpenTask(Task):

    task_type = "open"

    def __init__(self, filename):
        self.config = filename


class SplitLinesTask(Task):

    task_type = "split_lines"
    struct = struct.Struct("<QQ")

    def __init__(self, input, start, end):
        self.config = self.struct.pack(start, end)
        self.inputs = (input,)


class RunTask(Task):

    task_type = "run"

    def __init__(self, args, stdin=None, stdout=None, variable=None):
        if isinstance(args, str):
            args = args.split()
        else:
            args = map(str, args)
        self.args = args
        self.variable = variable
        self.stdout = None
        self.stdin = stdin
        self.stdout = stdout
        self.file_ins = []
        self.file_outs = []

    def map_file_in(self, task, filename):
        self.file_ins.append((task, filename))
        return self

    def map_file_out(self, filename):
        self.file_outs.append(filename)
        return self

    @property
    def inputs(self):
        if self.stdin is not None:
            inputs = [self.stdin]
        else:
            inputs = []

        for task, filename in self.file_ins:
            if task not in inputs:
                inputs.append(task)

        return inputs

    @property
    def config(self):
        msg = loomrun_pb2.Run()
        msg.args.extend(self.args)

        #  stdin
        if self.stdin is not None:
            mf = msg.maps.add()
            mf.filename = "+in"
            mf.input_index = 0
            mf.output_index = -2

        output_mapped = False
        for filename in self.file_outs:
            mf = msg.maps.add()
            mf.filename = filename
            mf.input_index = -2
            mf.output_index = -1
            output_mapped = True

        if not output_mapped:
            # stdout
            mf = msg.maps.add()
            mf.filename = "+out"
            mf.input_index = -2
            mf.output_index = -1

        inputs = self.inputs
        for task, filename in self.file_ins:
            mf = msg.maps.add()
            mf.filename = filename
            mf.input_index = inputs.index(task)
            mf.output_index = -2

        return msg.SerializeToString()


class Plan(object):

    TASK_ARRAY_MAKE = "array/make"
    TASK_ARRAY_GET = "array/get"
    u64 = struct.Struct("<Q")

    def __init__(self):
        self.tasks = []
        self.task_types = set()

    def add(self, task):
        assert task.id is None
        task.id = len(self.tasks)
        self.tasks.append(task)
        self.task_types.add(task.task_type)
        return task

    def task_const(self, data):
        return self.add(ConstTask(data))

    def task_open(self, filename):
        return self.add(OpenTask(filename))

    def task_merge(self, inputs):
        return self.add(MergeTask(inputs))

    def task_split_lines(self, input, start, end):
        return self.add(SplitLinesTask(input, start, end))

    def task_run(self, args, stdin=None, stdout=None, variable=None):
        return self.add(
            RunTask(args, stdin=stdin, stdout=stdout, variable=variable))

    def task_array_make(self, inputs):
        task = Task()
        task.task_type = self.TASK_ARRAY_MAKE
        task.inputs = inputs
        return self.add(task)

    def task_array_get(self, input, index):
        task = Task()
        task.task_type = self.TASK_ARRAY_GET
        task.inputs = (input,)
        task.config = self.u64.pack(index)
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
