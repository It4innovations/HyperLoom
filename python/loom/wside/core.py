import cloudpickle
import loom_c
import threading

class Context:

    MESSAGE_FORMAT = "PyTask id={}: {}"

    def __init__(self, task_id):
        self.task_id = task_id

    def log_debug(self, message):
        loom_c.log_debug(self.MESSAGE_FORMAT.format(self.task_id, message))

    def log_info(self, message):
        loom_c.log_info(self.MESSAGE_FORMAT.format(self.task_id, message))

    def log_warn(self, message):
        loom_c.log_warn(self.MESSAGE_FORMAT.format(self.task_id, message))

    def log_error(self, message):
        loom_c.log_error(self.MESSAGE_FORMAT.format(self.task_id, message))

    def log_critical(self, message):
        loom_c.log_critical(self.MESSAGE_FORMAT.format(self.task_id, message))

    def wrap(self, obj):
        return loom_c.wrap(obj)


def execute(fn_obj, data, inputs, task_id):
    params = cloudpickle.loads(data)
    if params:
        inputs = tuple(params) + inputs
    if isinstance(fn_obj, tuple):
        fn_obj, has_context = fn_obj
    else:
        has_context = False
    if has_context:
        context = Context(task_id)
        return fn_obj(context, *inputs)
    else:
        return fn_obj(*inputs)


unpickle_lock = threading.Lock()


def unpickle(data):
    with unpickle_lock:
        return cloudpickle.loads(data)