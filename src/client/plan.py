
import loomplan_pb2
import loomrun_pb2


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

    def map_file_out(self, filename):
        self.file_outs.append(filename)

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

    def task_merge(self, inputs):
        return self.add(MergeTask(inputs))

    def task_run(self, args, stdin=None, stdout=None, variable=None):
        return self.add(
            RunTask(args, stdin=stdin, stdout=stdout, variable=variable))

    def create_message(self):
        task_types = list(self.task_types)
        task_types.sort()

        msg = loomplan_pb2.Plan()
        msg.task_types.extend(task_types)

        for task in self.tasks:
            t = msg.tasks.add()
            task.set_message(t, task_types)
        return msg
