
# HACK! We need to fix this
import sys
import os
sys.path.insert(0,
                os.path.join(
                    os.path.dirname(__file__),
                    "..",
                    "client"))

import loomreport_pb2  # noqa
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
        graph = gv.Graph()
        symbols = self.symbols
        for i, task in enumerate(self.report_msg.plan.tasks):
            node = graph.node(i)
            node.label = symbols[task.task_type]
            for j in task.input_ids:
                graph.node(j).add_arc(node)
        return graph
