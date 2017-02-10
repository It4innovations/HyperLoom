
from ..pb import loomreport_pb2 as loomreport
from ..pb import loomcomm_pb2 as loomcomm
from .gv import Graph
import matplotlib.pyplot as plt
import pickle


def generate_colors(count):
    def linspace(start, stop, n):
        if n == 1:
            yield stop
            return
        h = float(stop - start) / (n - 1)
        for i in range(n):
            yield start + h * i
    return plt.cm.Set2(list(linspace(0, 1, count)))


def dot_color(color):
    r = int(color[0] * 256)
    g = int(color[1] * 256)
    b = int(color[2] * 256)
    return "#" + hex((r << 16) + (g << 8) + b)[2:]


class Report:

    def __init__(self, filename):
        with open(filename, "rb") as f:
            raw_data = f.read()

        self.report_msg = loomreport.Report()
        self.report_msg.ParseFromString(raw_data)

        self.symbols = [s.replace("loom", "L")
                        for s in self.report_msg.symbols]

    def label_counts(self):
        labels = {}
        tasks = self.report_msg.plan.tasks
        symbols = self.symbols
        for task in tasks:
            if task.label:
                key = task.label
                if ":" in key:
                    key = key.split(":")[0].strip()
            else:
                key = symbols[task.task_type]
            labels.setdefault(key, 0)
            labels[key] += 1
        return labels

    def collect_labels(self):
        labels = set()
        tasks = self.report_msg.plan.tasks
        symbols = self.symbols
        for task in tasks:
            if task.label:
                if ":" in task.label:
                    labels.add(task.label.split(":")[0])
                else:
                    labels.add(task.label)
            else:
                labels.add(symbols[task.task_type])

        label_groups = {}
        group_names = []
        labels = sorted(labels)

        for label in labels:
            if ":" in label:
                group = label.split(":")[0].strip()
            else:
                group = label
            try:
                index = group_names.index(group)
            except ValueError:
                index = len(group_names)
                group_names.append(group)
            label_groups[label] = index

        return label_groups, group_names

    def create_graph(self):
        TASK_START = loomcomm.Event.TASK_START
        graph = Graph()
        symbols = self.symbols

        task_workers = {}
        for event in self.report_msg.events:
            if event.type == TASK_START:
                task_workers[event.id] = event.worker_id

        label_groups, group_names = self.collect_labels()
        colors = [dot_color(c) for c in generate_colors(len(group_names))]

        for i, task in enumerate(self.report_msg.plan.tasks):
            node = graph.node(i)
            if task.label:
                label = task.label
            else:
                label = symbols[task.task_type]
            node.label = label
            if task_workers:
                node.label += "\nw={}".format(task_workers[i])

            node.fillcolor = colors[label_groups[label]]
            node.color = colors[label_groups[label]]
            for j in task.input_ids:
                graph.node(j).add_arc(node)
        return graph

    def get_event_rate_data(self, time_interval):
        counts = {}
        half = time_interval / 2
        for event in self.report_msg.events:
            time = (event.time // time_interval) * time_interval
            if time not in counts:
                counts[time] = 1
            else:
                counts[time] += 1
        result = sorted(counts.items())
        return [x[0] + half for x in result], [x[1] for x in result]

    def get_events_hline_data(self):
        TASK_START = loomcomm.Event.TASK_START
        TASK_END = loomcomm.Event.TASK_END
        workers = {}

        symbols = self.symbols
        label_groups, group_names = self.collect_labels()

        for event in self.report_msg.events:
            lst = workers.get(event.worker_id)
            if lst is None:
                lst = []
                workers[event.worker_id] = lst
            if event.type == TASK_END:
                for lst2 in lst:
                    if lst2[-1].type == TASK_START and lst2[-1].id == event.id:
                        lst2.append(event)
                        break
                else:
                    assert 0
            elif event.type == TASK_START:
                for lst2 in lst:
                    if lst2[-1].type == TASK_END:
                        lst2.append(event)
                        break
                else:
                    lst.append([event])

        end_time = self.report_msg.events[-1].time

        y = []
        xmin = []
        xmax = []
        colors = []
        y_labels = []

        y_unfinished = []
        xmin_unfinished = []
        xmax_unfinished = []
        colors_unfinished = []

        color_list = generate_colors(len(group_names))
        tasks = self.report_msg.plan.tasks

        index = 0
        for w_index, (worker, lst) in enumerate(sorted(workers.items())):
            y_labels.append("Worker {}".format(w_index))
            y_labels.extend([""] * len(lst))
            for lst2 in lst:
                count = len(lst2)
                if count % 2 == 1:
                    count -= 1
                    task = tasks[lst2[-1].id]
                    if task.label:
                        label = task.label
                    else:
                        label = symbols[task.task_type]
                    y_unfinished.append(index)
                    xmin_unfinished.append(lst2[-1].time)
                    xmax_unfinished.append(end_time)
                    colors_unfinished.append(color_list[label_groups[label]])

                for i in range(0, count, 2):
                    task = tasks[lst2[i].id]
                    if task.label:
                        label = task.label
                    else:
                        label = symbols[task.task_type]
                    y.append(index)
                    xmin.append(lst2[i].time)
                    xmax.append(lst2[i + 1].time)
                    colors.append(color_list[label_groups[label]])
                index += 1
            index += 1

        return ((y, xmin, xmax, colors),
                (y_unfinished, xmin_unfinished, xmax_unfinished,
                 colors_unfinished),
                y_labels,
                [(l, color_list[i])
                 for i, l in enumerate(group_names)])

    def get_events_hline_data_bokeh(self):
        TASK_START = loomcomm.Event.TASK_START
        TASK_END = loomcomm.Event.TASK_END
        workers = {}

        symbols = self.symbols
        label_groups, group_names = self.collect_labels()

        for event in self.report_msg.events:
            lst = workers.get(event.worker_id)
            if lst is None:
                lst = []
                workers[event.worker_id] = lst
            if event.type == TASK_END:
                for lst2 in lst:
                    if lst2[-1].type == TASK_START and lst2[-1].id == event.id:
                        lst2.append(event)
                        break
                else:
                    assert 0
            elif event.type == TASK_START:
                for lst2 in lst:
                    if lst2[-1].type == TASK_END:
                        lst2.append(event)
                        break
                else:
                    lst.append([event])

        end_time = self.report_msg.events[-1].time

        finished = {}
        unfinished = {}

        y_labels = []
        color_list = generate_colors(len(group_names))
        tasks = self.report_msg.plan.tasks

        index = 0
        for w_index, (worker, lst) in enumerate(sorted(workers.items())):
            y_labels.append("Worker {}".format(w_index))
            y_labels.extend([""] * len(lst))
            for lst2 in lst:
                count = len(lst2)
                if count % 2 == 1:
                    count -= 1
                    task = tasks[lst2[-1].id]
                    if task.label:
                        if ":" in task.label:
                            label = task.label.split(":")[0]
                        else:
                            label = task.label
                    else:
                        label = symbols[task.task_type]
                    if label not in unfinished:
                        unfinished[label] = ([], [], [], color_list[label_groups[label]])
                    unfinished[label][0].append(index)
                    unfinished[label][1].append(lst2[-1].time)
                    unfinished[label][2].append(end_time)

                for i in range(0, count, 2):
                    task = tasks[lst2[i].id]
                    if task.label:
                        if ":" in task.label:
                            label = task.label.split(":")[0]
                        else:
                            label = task.label
                    else:
                        label = symbols[task.task_type]
                    if label not in finished:
                        finished[label] = ([], [], [], color_list[label_groups[label]])
                    finished[label][0].append(index)
                    finished[label][1].append(lst2[i].time)
                    finished[label][2].append(lst2[i + 1].time)
                index += 1
            index += 1

        return finished, unfinished

    def get_ctasks_data(self):
        TASK_START = loomcomm.Event.TASK_START
        tasks = self.report_msg.plan.tasks
        symbols = self.symbols
        data = {}
        for event in self.report_msg.events:
            if event.type == TASK_START:
                task = tasks[event.id]
                if task.label:
                    label = task.label.split(":")[0]
                else:
                    label = symbols[task.task_type]
                d = data.get(label)
                if d is None:
                    d = [[], []]
                    data[label] = d
                d[0].append(event.time)
                if d[1]:
                    d[1].append(d[1][-1] + 1)
                else:
                    d[1].append(1)
        return data

    def get_ctime_data(self):
        TASK_START = loomcomm.Event.TASK_START
        TASK_END = loomcomm.Event.TASK_END
        tasks = self.report_msg.plan.tasks
        symbols = self.symbols
        data = {}
        task_start_ts = {}
        for event in self.report_msg.events:
            if event.type == TASK_START:
                task_start_ts[event.id] = event.time
            if event.type == TASK_END:
                task = tasks[event.id]
                if task.label:
                    label = task.label.split(":")[0]
                else:
                    label = symbols[task.task_type]
                d = data.get(label)
                if d is None:
                    d = [[], []]
                    data[label] = d
                d[0].append(event.time)
                duration = event.time - task_start_ts[event.id]
                if d[1]:
                    d[1].append(d[1][-1] + duration)
                else:
                    d[1].append(duration)
                task_start_ts.pop(event.id)
        return data

    def get_ctransfer_data(self, intra):
        SEND_START = loomcomm.Event.SEND_START
        TASK_END = loomcomm.Event.TASK_END
        sizes = {}
        label_groups, group_names = self.collect_labels()
        symbols = self.symbols

        group_names.append("SUM")
        results = []
        for name in group_names:
            results.append(([0], [0], name))

        total = results[-1]

        for event in self.report_msg.events:
            if event.type == TASK_END:
                sizes[event.id] = event.size
            elif event.type == SEND_START:
                if (intra and event.target_worker_id == -1) or \
                   (not intra and event.target_worker_id != -1):
                    continue
                task_id = event.id
                total[0].append(event.time)
                total[1].append(total[1][-1] + sizes[task_id])

                task = self.report_msg.plan.tasks[task_id]
                if task.label:
                    label = task.label
                else:
                    label = symbols[task.task_type]
                group = label_groups[label]
                data = results[group]
                data[0].append(event.time)
                data[1].append(data[1][-1] + sizes[event.id])
        return results

    def get_btime_data(self):
        TASK_START = loomcomm.Event.TASK_START
        TASK_END = loomcomm.Event.TASK_END
        tasks = self.report_msg.plan.tasks
        symbols = self.symbols
        data = {}
        task_start_ts = {}
        for event in self.report_msg.events:
            if event.type == TASK_START:
                task_start_ts[event.id] = event.time
            if event.type == TASK_END:
                task = tasks[event.id]
                if task.label:
                    label = task.label.split(":")[0]
                else:
                    label = symbols[task.task_type]
                d = data.get(label)
                if d is None:
                    d = []
                    data[label] = d
                duration = event.time - task_start_ts[event.id]
                d.append(duration)
                task_start_ts.pop(event.id)
        return data

    def get_json_data(self):
        tasks = self.report_msg.plan.tasks
        data = []
        symbols = self.symbols
        task_start_ts = {}
        for event in self.report_msg.events:
            task = tasks[event.id]
            if task.metadata:
                d = pickle.loads(tasks[event.id].metadata)
            else:
                d = {}
            if task.label:
                d["task_type"] = task.label.split(":")[0]
            else:
                d["task_type"] = symbols[task.task_type]
            d["id"] = event.id
            d["time"] = event.time
            d["worker_id"] = event.worker_id
            d["target_worker_id"] = event.target_worker_id
            d["size"] = event.size
            if event.type == loomcomm.Event.TASK_START:
                task_start_ts[event.id] = event.time
                d["event_type"] = "task_start"
            elif event.type == loomcomm.Event.TASK_END:
                d["duration"] = event.time - task_start_ts[event.id]
                task_start_ts.pop(event.id)
                d["event_type"] = "task_end"
            elif event.type == loomcomm.Event.SEND_START:
                d["event_type"] = "send_start"
            elif event.type == loomcomm.Event.SEND_END:
                d["event_type"] = "send_end"
            else:
                d["event_type"] = event.type
            data.append(d)
        return data

    def get_html_data(self, dhm_conf):
        tasks = self.report_msg.plan.tasks
        symbols = self.symbols
        task_start_ts = {}
        event_rate_data = {"time": [], "task_type": []}
        task_duration_data = {"duration": [], "task_type": []}
        c_task_duration_data = {}

        dhm_params = dhm_conf["params"]
        dhm_tt = dhm_conf["task_type"]
        dhm_data = {dhm_params[0]: [], dhm_params[1]: [], "values": [], "labels": []}

        event_idx = 1
        event_count = len(self.report_msg.events)
        for event in self.report_msg.events:
            if event_idx % 10000 == 0:
                print("Processing Loom report: event {}/{} ({:.1f}%)".format(
                      event_idx, event_count, 100 * float(event_idx) / event_count))
            event_idx += 1
            task = tasks[event.id]
            if task.label:
                task_type = task.label.split(":")[0]
            else:
                task_type = symbols[task.task_type]
            if event.type == loomcomm.Event.TASK_START:
                task_start_ts[event.id] = event.time
                event_type = "task_start"
            elif event.type == loomcomm.Event.TASK_END:
                duration = event.time - task_start_ts[event.id]

                # Task duration data
                task_duration_data["duration"].append(duration)
                task_duration_data["task_type"].append(task_type)

                # Cummulative duration data
                if task_type not in c_task_duration_data:
                    c_task_duration_data[task_type] = [[event.time], [duration]]
                else:
                    c_task_duration_data[task_type][0].append(event.time)
                    c_task_duration_data[task_type][1].append(
                        c_task_duration_data[task_type][1][-1] + duration)

                # Duration HeatMap data
                if task_type == dhm_tt and task.metadata:
                    task_metadata = pickle.loads(task.metadata)
                    dhm_data[dhm_params[0]].append(str(task_metadata[dhm_params[0]]))
                    dhm_data[dhm_params[1]].append(str(task_metadata[dhm_params[1]]))
                    dhm_data["values"].append(duration)
                    dhm_data["labels"].append("{}={} {}={}".format(
                        dhm_params[0], task_metadata[dhm_params[0]],
                        dhm_params[1], task_metadata[dhm_params[1]]))

                task_start_ts.pop(event.id)
                event_type = "task_end"
            elif event.type == loomcomm.Event.SEND_START:
                event_type = "send_start"
            elif event.type == loomcomm.Event.SEND_END:
                event_type = "send_end"
            else:
                event_type = event.type

            # Event rate data
            event_rate_data["time"].append(event.time)
            event_rate_data["task_type"].append(task_type)

        return event_rate_data, task_duration_data, c_task_duration_data, dhm_data
