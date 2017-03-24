import pickle

from .task import Task
from .future import Future


class Plan(object):

    def __init__(self, id_base):
        self.tasks = {}
        self.id_counter = id_base

    def add(self, task):
        def give_id(task):
            if isinstance(task, Task):
                if task not in self.tasks:
                    for t in task.inputs:
                        give_id(t)
                    self.tasks[task] = self.id_counter
                    self.id_counter += 1
            elif isinstance(task, Future):
                if not task.active():
                    raise Exception("Inactive future used in plan")
            else:
                raise Exception("Invalid object {} in plan".format(repr(task)))
        give_id(task)

    def collect_symbols(self):
        symbols = set()
        for task in self.tasks:
            if task.resource_request:
                symbols.update(task.resource_request.names)
            symbols.add(task.task_type)
        return symbols

    def get_id(self, task):
        id = self.tasks.get(task)
        if id is not None:
            return id
        return task.task_id

    def set_message(self, msg, symbols, results,
                    include_metadata=False):
        requests = set()

        # Linearize tasks
        tasks = list(task for task, index in
                     sorted(self.tasks.items(), key=lambda p: p[1]))

        # Gather requests
        requests = set()
        for task in self.tasks:
            if task.resource_request:
                requests.add(task.resource_request)
        requests = list(requests)

        # Build requests
        for request in requests:
            r = msg.resource_requests.add()
            request.set_message(r, symbols)

        # Build tasks
        for task in tasks:
            msg_t = msg.tasks.add()
            config = task.config
            if isinstance(config, str):
                config = bytes(config, encoding="utf-8")
            msg_t.config = config
            msg_t.task_type = symbols[task.task_type]
            msg_t.input_ids.extend(self.get_id(t) for t in task.inputs)
            msg_t.result = task in results
            if task.resource_request:
                msg_t.resource_request_index = \
                    requests.index(task.resource_request)
            if include_metadata and task.label:
                msg_t.label = task.label
            if include_metadata and task.metadata is not None:
                msg_t.metadata = pickle.dumps(task.metadata)

        return msg
