
import loomplan_pb2


POLICY_STANDARD = loomplan_pb2.Task.POLICY_STANDARD
POLICY_SIMPLE = loomplan_pb2.Task.POLICY_SIMPLE
POLICY_SCHEDULER = loomplan_pb2.Task.POLICY_SCHEDULER


class Task(object):

    task_type = None
    inputs = ()
    id = None
    config = ""
    policy = POLICY_STANDARD
    resource_request = None
    label = None

    def set_message(self, msg, symbols, requests):
        msg.config = self.config
        msg.task_type = symbols[self.task_type]
        msg.input_ids.extend(t.id for t in self.inputs)
        msg.policy = self.policy
        if self.resource_request:
            msg.resource_request_index = requests.index(self.resource_request)
        if self.label:
            msg.label = self.label


class ResourceRequest(object):

    def __init__(self):
        self.resources = {}

    def add_resource(self, name, value):
        self.resources[name] = value

    @property
    def names(self):
        return self.resources.keys()

    def set_message(self, msg, symbols):
        for name, value in self.resources.items():
            r = msg.resources.add()
            r.resource_type = symbols[name]
            r.value = value

    def __eq__(self, other):
        if not isinstance(other, ResourceRequest):
            return False
        return self.resources == other.resources

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash(tuple(sorted(self.resources.items())))


class Plan(object):

    def __init__(self):
        self.tasks = []

    def add(self, task):
        assert task.id is None
        task.id = len(self.tasks)
        self.tasks.append(task)
        return task

    def collect_symbols(self):
        symbols = set()
        for task in self.tasks:
            if task.resource_request:
                symbols.update(task.resource_request.names)
            symbols.add(task.task_type)
        return symbols

    def set_message(self, msg, symbols):
        requests = set()
        for task in self.tasks:
            if task.resource_request:
                requests.add(task.resource_request)
        requests = list(requests)

        for request in requests:
            r = msg.resource_requests.add()
            request.set_message(r, symbols)

        for task in self.tasks:
            t = msg.tasks.add()
            task.set_message(t, symbols, requests)
        return msg
