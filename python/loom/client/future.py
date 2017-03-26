
class Future(object):
    """
    Represents a remote computation in server
    """

    def __init__(self, client, task_id):
        self.task_id = task_id
        self.remote_status = "running"
        self.client = client
        self.has_result = False
        self._result = None

    def finished(self):
        """
        Returns ``True`` if task finished but not released
        """
        return self.remote_status == "finished"

    def active(self):
        """
        Returns ``True`` if task running or finished, but not released.
        """
        return self.remote_status == "running" \
            or self.remote_status == "finished"

    def released(self):
        """
        Returns ``True`` if task was released.
        """
        return self.remote_status == "released" \
            or self.remote_status == "cancel"

    def running(self):
        """
        Returns ``True`` if task is running.
        """
        return self.remote_status == "running"

    def wait(self):
        """
        Waits until the future is not finished.
        """
        self.client.wait_one(self)

    def fetch(self):
        """
        Waits until the future is not finished, then downloads
        the result from workers.
        """
        return self.client.fetch_one(self)

    def gather(self):
        """
        The same as ``fetch()`` followed by ``release()``.
        """
        return self.client.gather_one(self)

    def set_finished(self):
        assert self.remote_status == "running"
        self.remote_status = "finished"

    def set_result(self, data):
        assert self.remote_status == "finished"
        self.has_result = True
        self._result = data

    def release(self):
        """
        Remove result from workers
        (also called in ``__del__`` if not called sooner)
        """
        self.client.release_one(self)

    def __del__(self):
        self.client.release_one(self)

    def __repr__(self):
        return "<Future task_id={} {}{}>".format(
            self.task_id, self.remote_status,
            " R" if self.has_result else "")
