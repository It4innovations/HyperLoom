class LoomException(Exception):
    """Base class for Loom exceptions"""
    pass


class TaskFailed(LoomException):
    """Exception when scheduler informs about failure of a task"""

    def __init__(self, id, worker, error_msg):
        self.id = id
        self.worker = worker
        self.error_msg = error_msg
        message = "Task id={} failed: {}".format(id, error_msg)
        LoomException.__init__(self, message)


class LoomError(LoomException):
    """Generic error in Loom system"""
    pass
