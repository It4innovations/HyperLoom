
# HACK! We need to fix this
import sys
import os
sys.path.insert(0,
                os.path.join(
                    os.path.dirname(__file__),
                    "..",
                    "client"))

import loomreport_pb2  # noqa
import loomcomm_pb2  # noqa
import gv  # noqas
import matplotlib.pyplot as plt  # noqa


def generate_colors(count):
    def linspace(start, stop, n):
        if n == 1:
            yield stop
            return
        h = float(stop - start) / (n - 1)
        for i in range(n):
            yield start + h * i
    return plt.cm.Set2(list(linspace(0, 1, 12)))


def dot_color(color):
    r = int(color[0] * 256)
    g = int(color[1] * 256)
    b = int(color[2] * 256)
    return "#" + hex((r << 16) + (g << 8) + b)[2:]


class Report:

    def __init__(self, filename):
        with open(filename) as f:
            raw_data = f.read()

        self.report_msg = loomreport_pb2.Report()
        self.report_msg.ParseFromString(raw_data)

        self.symbols = [s.replace("loom", "L")
                        for s in self.report_msg.symbols]

    def collect_labels(self):
        labels = set()
        tasks = self.report_msg.plan.tasks
        symbols = self.symbols
        for task in tasks:
            if task.label:
                labels.add(task.label)
            else:
                labels.add(symbols[task.task_type])
        return sorted(labels)

    def create_graph(self):
        TASK_START = loomcomm_pb2.Event.TASK_START
        graph = gv.Graph()
        symbols = self.symbols

        task_workers = {}
        for event in self.report_msg.events:
            if event.type == TASK_START:
                task_workers[event.id] = event.worker_id

        labels = self.collect_labels()
        colors = [dot_color(c) for c in generate_colors(len(labels))]

        for i, task in enumerate(self.report_msg.plan.tasks):
            node = graph.node(i)
            if task.label:
                label = task.label
            else:
                label = symbols[task.task_type]
            node.label = label
            if task_workers:
                node.label += "\nw={}".format(task_workers[i])

            node.fillcolor = colors[labels.index(label)]
            node.color = colors[labels.index(label)]
            for j in task.input_ids:
                graph.node(j).add_arc(node)
        return graph

    def get_events_hline_data(self):
        TASK_START = loomcomm_pb2.Event.TASK_START
        TASK_END = loomcomm_pb2.Event.TASK_END
        workers = {}

        symbols = self.symbols
        labels = self.collect_labels()

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

        y = []
        xmin = []
        xmax = []
        colors = []
        y_labels = []

        color_list = generate_colors(len(labels))
        tasks = self.report_msg.plan.tasks

        index = 0
        for w_index, (worker, lst) in enumerate(sorted(workers.items())):
            y_labels.append("Worker {}".format(w_index))
            y_labels.extend([""] * len(lst))
            for lst2 in lst:
                for i in xrange(0, len(lst2), 2):
                    y.append(index)
                    xmin.append(lst2[i].time)
                    xmax.append(lst2[i + 1].time)
                    task = tasks[lst2[i].id]
                    if task.label:
                        label = task.label
                    else:
                        label = symbols[task.task_type]
                    colors.append(color_list[labels.index(label)])
                index += 1
            index += 1

        return (y,
                xmin,
                xmax,
                colors,
                y_labels,
                [(l, color_list[i])
                 for i, l in enumerate(labels)])
