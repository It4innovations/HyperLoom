
from ..pb import loomreport_pb2 as loomreport
from ..pb import loomcomm_pb2 as loomcomm
from .gv import Graph
import matplotlib.pyplot as plt


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
                # if ":" in key:
                #     key = key.split(":")[0].strip()
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
            else:
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

    def get_cumulative_data(self):
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
                if label not in data:
                    data[label] = [[], []]
                data[label][0].append(event.time)
                if data[label][1]:
                    data[label][1].append(data[label][1][-1] + 1)
                else:
                    data[label][1].append(1)
        return data
