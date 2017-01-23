import cloudpickle
import loom_c


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


def unpack_and_execute(data, inputs, task_id):
    obj, has_context, params = cloudpickle.loads(data)
    if params:
        inputs = tuple(params) + inputs
    if has_context:
        context = Context(task_id)
        return obj(context, *inputs)
    else:
        return obj(*inputs)
