
import os
from loom.pb import loomcomm_pb2 as loomcomm
import pandas as pd


def split_lines(f):
    for line in f:
        yield line.split()


def to_int(value):
    return int(value, 16)


def index_dict(values):
    result = {}
    for i, v in enumerate(values):
        result[v] = i
    return result


class Task:

    __slots__ = ['start_time', 'end_time', 'enabled_time', 'task_type', 'worker_id', 'label']

    def __init__(self):
        self.start_time = None
        self.end_time = None
        self.enabled_time = None
        self.task_type = None
        self.worker_id = None
        self.label = None

    @property
    def name(self):
        return str(self.task_type)

    @property
    def group_name(self):
        label = self.label
        if label:
            if ":" not in label:
                return label
            else:
                return label.split(":")[0]
        else:
            return self.task_type


class Worker:

    def __init__(self, worker_id, address):
        self.worker_id = worker_id
        self.address = address
        self.sends = None
        self.recvs = None
        self.monitoring_times = []
        self.monitoring_cpu = []
        self.monitoring_mem = []


class Report:

    def __init__(self, trace_path):
        self.trace_path = trace_path

        self.workers = {}
        self.tasks = {}
        self.symbols = []
        self.id_base = None
        self.scheduler_times = None

        self.parse_server_file()
        for worker_id in self.workers:
            self.parse_worker_file(worker_id)
        self.parse_plan_file()

        print("Loaded {} tasks".format(len(self.tasks)))

    @property
    def worker_list(self):
        result = list(self.workers.values())
        result.sort(key=lambda w: w.worker_id)
        return result

    def all_sends(self):
        sends = [worker.sends for worker in self.workers.values()]
        return pd.concat(sends)

    def check(self):
        if not os.path.isdir(self.trace_path):
            return "Directory '{}' not found".format(self.trace_path)
        return None

    def get_filename(self, filename):
        return os.path.join(self.trace_path, filename)

    def parse_server_file(self):
        filename = self.get_filename("server.ltrace")
        print("Reading", filename, "...")
        start_time = []
        end_time = []
        with open(filename) as f:
            it = split_lines(f)
            assert next(it) == ["TRACE", "server"]
            assert next(it) == ["VERSION", "0"]

            time = 0
            for line in it:
                command = line[0]
                if command == "T":
                    time = to_int(line[1])
                elif command == "s":
                    start_time.append(time)
                elif command == "e":
                    end_time.append(time)
                elif command == "WORKER":
                    worker_id = to_int(line[1])
                    self.workers[worker_id] = Worker(worker_id, line[2])
                elif command == "SYMBOL":
                    self.symbols.append(line[1])
                elif command == "SUBMIT":
                    self.id_base = to_int(line[1])
                else:
                    raise Exception("Unknown line: {}".format(line))
        if self.id_base is None:
            raise Exception("No submit occurs")
        self.scheduler_times = pd.DataFrame(
            {"start_time": start_time, "end_time": end_time})

    def get_task(self, task_id):
        return self.tasks[task_id]

    def parse_worker_file(self, worker_id):
        filename = self.get_filename("worker-{}.ltrace".format(worker_id))
        print("Reading", filename, "...")
        worker = self.workers[worker_id]

        sends = []
        recvs = []

        with open(filename) as f:
            it = split_lines(f)
            assert next(it) == ["TRACE", "worker"]
            assert next(it) == ["VERSION", "0"]

            time = 0
            for line in it:
                command = line[0]
                if command == "T":  # TIME
                    time = to_int(line[1])
                elif command == "S":  # START
                    task = self._get_task(to_int(line[1]))
                    task.start_time = time
                    task.worker_id = worker_id
                elif command == "F":  # FINISH
                    task = self._get_task(to_int(line[1]))
                    task.end_time = time
                elif command == "M":
                    worker.monitoring_times.append(time)
                    worker.monitoring_cpu.append(to_int(line[1]))
                    worker.monitoring_mem.append(to_int(line[2]))
                elif command == "D":
                    sends.append((time, to_int(line[1]), to_int(line[2])))
                elif command == "R":
                    recvs.append((time, to_int(line[1]), to_int(line[2])))
                else:
                    raise Exception("Unknown line: {}".format(line))

            columns = ("time", "id", "data_size")
            worker.sends = pd.DataFrame(sends, columns=columns)
            worker.recvs = pd.DataFrame(recvs, columns=columns)

    def _get_task(self, task_id):
        task = self.tasks.get(task_id)
        if task is not None:
            return task
        task = Task()
        self.tasks[task_id] = task
        return task

    def parse_plan_file(self):
        filename = self.get_filename("0.plan")
        print("Reading", filename, "...")
        id_base = self.id_base
        with open(filename, "br") as f:
            request = loomcomm.ClientRequest()
            request.ParseFromString(f.read())
            for i, pt in enumerate(request.plan.tasks):
                task = self._get_task(i + id_base)
                times = [self._get_task(i2 + id_base).end_time
                         for i2 in pt.input_ids]
                if all(time is not None for time in times):
                    if times:
                        task.enabled_time = max(times)
                    else:
                        task.enabled_time = 0
                task.task_type = self.symbols[pt.task_type]
                task.label = pt.label

    def get_pending_tasks(self):
        def create_items():
            for t in self.tasks.values():
                group_name = t.group_name
                yield t.enabled_time, 1, group_name
                yield t.start_time, -1, group_name
        return pd.DataFrame(create_items(),
                            columns=["time", "change", "group"])

    def get_group_names(self):
        return sorted(set(t.group_name for t in self.tasks.values()))

    def get_task_frame(self):
        return pd.DataFrame(
            ((t.start_time, t.end_time, t.group_name)
             for t in self.tasks.values()),
            columns=("start_time", "end_time", "group"))

    def get_counts(self):
        group_names = self.get_group_names()
        group_index = index_dict(group_names)
        results = [0 for name in group_names]

        for task in self.tasks.values():
            index = group_index[task.group_name]
            results[index] += 1

        return results, group_names

    def get_timelines(self):
        worker_lines = {}
        group_names = self.get_group_names()
        group_index = index_dict(group_names)

        for worker_id in self.workers:
            worker_lines[worker_id] = []

        tasks = list(self.tasks.values())
        tasks.sort(key=lambda task: task.start_time)

        for task in tasks:
            lines = worker_lines[task.worker_id]
            start_time = task.start_time
            for starts, ends, gids in lines:
                if ends[-1] <= start_time:
                    starts.append(start_time)
                    ends.append(task.end_time)
                    gids.append(group_index[task.group_name])
                    break
            else:
                lines.append(([start_time],
                              [task.end_time],
                              [group_index[task.group_name]]))

        scheduler = len(group_names)
        group_names.append("scheduler")

        server_lines = [
            (self.scheduler_times.start_time,
             self.scheduler_times.end_time,
             [scheduler for x in range(len(self.scheduler_times))])
        ]
        worker_lines[-1] = server_lines

        return worker_lines, group_names
