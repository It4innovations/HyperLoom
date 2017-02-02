import pickle


class Plan(object):

    def __init__(self):
        self.tasks = {}

    def add(self, task):
        def give_id(task):
            if task not in self.tasks:
                for t in task.inputs:
                    give_id(t)
                self.tasks[task] = len(self.tasks)
        give_id(task)

    def collect_symbols(self):
        symbols = set()
        for task in self.tasks:
            if task.resource_request:
                symbols.update(task.resource_request.names)
            symbols.add(task.task_type)
        return symbols

    def set_message(self, msg, symbols, include_metadata=False):
        requests = set()

        # Linearize tasks
        tasks = [None] * len(self.tasks)
        for task, index in self.tasks.items():
            tasks[index] = task

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
            msg_t.input_ids.extend(self.tasks[t] for t in task.inputs)
            msg_t.policy = task.policy
            if task.resource_request:
                msg_t.resource_request_index = \
                    requests.index(task.resource_request)
            if include_metadata and task.label:
                msg_t.label = task.label
            if include_metadata and task.metadata is not None:
                msg_t.metadata = pickle.dumps(task.metadata)

        return msg
