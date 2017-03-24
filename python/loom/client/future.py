
class Future(object):
    """
    There are 5 states of future:
    - running
    - finished
    - canceled
    - released
    - error
    """

    def __init__(self, client, task_id):
        self.task_id = task_id
        self.remote_status = "running"
        self.client = client
        self.has_result = False
        self._result = None

    def finished(self):
        return self.remote_status == "finished"

    def active(self):
        return self.remote_status == "running" \
            or self.remote_status == "finished"

    def released(self):
        return self.remote_status == "released" \
            or self.remote_status == "cancel"

    def running(self):
        return self.remote_status == "running"

    def wait(self):
        self.client.wait_one(self)

    def fetch(self):
        return self.client.fetch_one(self)

    def gather(self):
        return self.client.gather_one(self)

    def set_finished(self):
        assert self.remote_status == "running"
        self.remote_status = "finished"

    def set_result(self, data):
        assert self.remote_status == "finished"
        self.has_result = True
        self._result = data

    def release(self):
        self.client.release_one(self)

    def __del__(self):
        self.client.release_one(self)

    def __repr__(self):
        return "<Future task_id={} {}{}>".format(
            self.task_id, self.remote_status,
            " R" if self.has_result else "")
