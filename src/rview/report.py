
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


class Report:

    def __init__(self, filename):
        with open(filename) as f:
            raw_data = f.read()

        self.report_msg = loomreport_pb2.Report()
        self.report_msg.ParseFromString(raw_data)

        self.symbols = [s.replace("loom", "L")
                        for s in self.report_msg.symbols]

    def create_graph(self):
        TASK_START = loomcomm_pb2.Event.TASK_START
        graph = gv.Graph()
        symbols = self.symbols
        colors = ["red", "green", "blue", "yellow", "orange", "pink"]

        task_workers = {}
        for event in self.report_msg.events:
            if event.type == TASK_START:
                task_workers[event.id] = event.worker_id

        for i, task in enumerate(self.report_msg.plan.tasks):
            node = graph.node(i)
            node.label = symbols[task.task_type]
            if task_workers:
                node.color = colors[task_workers[i]]
            for j in task.input_ids:
                graph.node(j).add_arc(node)
        return graph

    def get_events_hline_data(self):
        TASK_START = loomcomm_pb2.Event.TASK_START
        TASK_END = loomcomm_pb2.Event.TASK_END
        workers = {}

        symbol_ids = set()

        tasks = self.report_msg.plan.tasks
        for task in tasks:
            symbol_ids.add(task.task_type)
        symbols = list(symbol_ids)
        symbols.sort(key=lambda x: self.symbols[x])

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
        labels = []

        color_list = ["red", "green", "blue", "pink", "orange"]

        index = 0
        for w_index, (worker, lst) in enumerate(sorted(workers.items())):
            labels.append("Worker {}".format(w_index))
            labels.extend([""] * len(lst))
            for lst2 in lst:
                for i in xrange(0, len(lst2), 2):
                    y.append(index)
                    xmin.append(lst2[i].time)
                    xmax.append(lst2[i + 1].time)
                    task = tasks[lst2[i].id]
                    colors.append(color_list[symbols.index(task.task_type)])
                index += 1
            index += 1

        return (y,
                xmin,
                xmax,
                colors,
                labels,
                [(self.symbols[symbol_id], color_list[symbols.index(symbol_id)])
                 for symbol_id in symbols])
