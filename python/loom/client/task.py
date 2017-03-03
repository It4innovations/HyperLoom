
from ..pb import comm_pb2


POLICY_STANDARD = comm_pb2.Task.POLICY_STANDARD
POLICY_SIMPLE = comm_pb2.Task.POLICY_SIMPLE
POLICY_SCHEDULER = comm_pb2.Task.POLICY_SCHEDULER


class Task(object):

    task_type = None
    inputs = ()
    config = ""
    policy = POLICY_STANDARD
    resource_request = None
    label = None
    metadata = None

    def __repr__(self):
        if self.label:
            label = self.label
        else:
            label = self.task_type
        return "<Task '{}'>".format(label)


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
