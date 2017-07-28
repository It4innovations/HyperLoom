
import os
from loom.pb import comm_pb2
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

    __slots__ = ['start_time', 'end_time', 'enabled_time',
                 'task_type', 'worker_id', 'label']

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


def _get_task(tasks, task_id):
    task = tasks.get(task_id)
    if task is not None:
        return task
    task = Task()
    tasks[task_id] = task
    return task


class Worker:

    def __init__(self, worker_id, address):
        self.worker_id = worker_id
        self.address = address
        self.sends = None
        self.recvs = None
        self.monitoring = None


class Report:

    def __init__(self, trace_path):
        self.trace_path = trace_path

        self.workers = {}  # by ID
        self.workers_by_addr = {}  # by address
        self.symbols = []
        self.id_bases = []
        self.scheduler_times = None

        tasks = {}
        self.parse_server_file(tasks)
        for worker_id in self.workers:
            self.parse_worker_file(worker_id, tasks)
        for id_base in self.id_bases:
            self.parse_plan_file(tasks, id_base)
        print("Loaded {} tasks".format(len(tasks)))

        task_frame = pd.DataFrame(
            ((task_id, t.start_time, t.end_time, t.enabled_time,
              t.worker_id, t.group_name)
             for task_id, t in tasks.items()),
            columns=("task_id", "start_time", "end_time", "enabled_time",
                     "worker", "group"))
        task_frame.set_index("task_id", inplace=True, verify_integrity=True)
        task_frame["duration"] = task_frame.end_time - task_frame.start_time
        task_frame.sort_values("end_time", inplace=True)
        self.task_frame = task_frame

        group_names = task_frame.group.unique()
        group_names.sort()
        self.group_names = group_names

        self.end_time = max(task_frame["start_time"].max(),
                            task_frame["end_time"].max(),
                            max(w.monitoring["time"].max()
                                for w in self.workers.values()))

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

    def parse_server_file(self, tasks):
        filename = self.get_filename("server.ltrace")
        print("Reading", filename, "...")
        start_time = []
        end_time = []
        with open(filename) as f:
            it = split_lines(f)
            assert next(it) == ["TRACE", "server"]
            assert next(it)[0] == "VERSION"

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
                    worker = Worker(worker_id, line[2])
                    self.workers[worker_id] = worker
                    self.workers_by_addr[line[2]] = worker
                elif command == "SYMBOL":
                    self.symbols.append(line[1])
                elif command == "SUBMIT":
                    self.id_bases.append(to_int(line[1]))
                else:
                    raise Exception("Unknown line: {}".format(line))
        if not self.id_bases:
            raise Exception("No submit occurs")
        if len(start_time) == len(end_time) + 1:
            start_time.pop()
        self.scheduler_times = pd.DataFrame(
            {"start_time": start_time, "end_time": end_time})

    def parse_worker_file(self, worker_id, tasks):
        filename = self.get_filename("worker-{}.ltrace".format(worker_id))
        print("Reading", filename, "...")
        worker = self.workers[worker_id]

        sends = []
        recvs = []

        with open(filename) as f:
            it = split_lines(f)
            assert next(it) == ["TRACE", "worker"]
            assert next(it) == ["VERSION", "0"]

            monitoring_times = []
            monitoring_cpu = []
            monitoring_mem = []
            time = 0
            for line in it:
                command = line[0]
                if command == "T":  # TIME
                    time = to_int(line[1])
                elif command == "S":  # START
                    task = _get_task(tasks, to_int(line[1]))
                    task.start_time = time
                    task.worker_id = worker_id
                elif command == "F":  # FINISH
                    task = _get_task(tasks, to_int(line[1]))
                    task.end_time = time
                elif command == "M":
                    monitoring_times.append(time)
                    monitoring_cpu.append(to_int(line[1]))
                    monitoring_mem.append(to_int(line[2]))
                elif command == "D":
                    w = self.workers_by_addr.get(line[3])
                    if w is not None:
                        w = w.worker_id
                    else:
                        w = -1
                    sends.append((time, to_int(line[1]), to_int(line[2]), w))
                elif command == "R":
                    w = self.workers_by_addr.get(line[3])
                    if w is not None:
                        w = w.worker_id
                    else:
                        w = -1
                        print("WARNING! Received data from dummy worker.")
                    recvs.append((time, to_int(line[1]), to_int(line[2]), w))
                else:
                    raise Exception("Unknown line: {}".format(line))

            columns = ("time", "id", "data_size", "peer_id")
            worker.sends = pd.DataFrame(sends, columns=columns)
            worker.recvs = pd.DataFrame(recvs, columns=columns)
            worker.monitoring = pd.DataFrame({"time": monitoring_times,
                                              "cpu": monitoring_cpu,
                                              "mem": monitoring_mem})

    def parse_plan_file(self, tasks, id_base):
        filename = self.get_filename("{}.plan".format(id_base))
        print("Reading", filename, "...")
        with open(filename, "br") as f:
            request = comm_pb2.ClientRequest()
            request.ParseFromString(f.read())
            for i, pt in enumerate(request.plan.tasks):
                task = _get_task(tasks, i + id_base)
                times = [_get_task(tasks, i2).end_time
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
            for row in self.task_frame.itertuples(index=False):
                group = row.group
                yield row.enabled_time, 1, group
                yield row.start_time, -1, group
        frame = pd.DataFrame(create_items(),
                             columns=["time", "change", "group"])
        frame.sort_values("time", inplace=True)
        return frame

    def get_timelines(self):
        group_names = list(self.group_names)
        group_index = index_dict(group_names)

        worker_lines = {}

        for worker_id in self.workers:
            worker_lines[worker_id] = []

        frame = pd.DataFrame(
            self.task_frame[["start_time", "end_time", "group", "worker"]])
        frame.sort_values("start_time", inplace=True)

        for row in frame.itertuples(index=False):
            lines = worker_lines[row.worker]
            for starts, ends, gids in lines:
                if ends[-1] <= row.start_time:
                    starts.append(row.start_time)
                    ends.append(row.end_time)
                    gids.append(group_index[row.group])
                    break
            else:
                lines.append(([row.start_time],
                              [row.end_time],
                              [group_index[row.group]]))

        scheduler = len(group_names)
        group_names.append("scheduler")

        server_lines = [
            (self.scheduler_times.start_time,
             self.scheduler_times.end_time,
             [scheduler for x in range(len(self.scheduler_times))])
        ]
        worker_lines[-1] = server_lines
        return worker_lines, group_names
