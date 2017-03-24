
from .connection import Connection
from .future import Future
from .plan import Plan
from .task import Task

from ..pb.comm_pb2 import Register
from ..pb.comm_pb2 import ClientRequest, ClientResponse

import socket
import struct
import cloudpickle
import os

LOOM_PROTOCOL_VERSION = 2


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


class Client(object):
    """The class that serves for connection to the server and submitting tasks

    Args:
        address (str): Address of the server
        port(port): TCP port of the server

    """

    def __init__(self, address, port=9010):
        self.server_address = address
        self.server_port = port

        self.trace_path = None
        self.symbols = None
        self.array_id = None
        self.rawdata_id = None

        self.submit_id = 0
        self.futures = {}
        self.n_finished_tasks = 0

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((address, port))
        self.connection = Connection(s)

        msg = Register()
        msg.type = Register.REGISTER_CLIENT
        msg.protocol_version = LOOM_PROTOCOL_VERSION
        self._send_message(msg)

        while self.symbols is None:
            self._read_symbols()

    def get_stats(self):
        """Ask server for basic statistic informations"""
        msg = ClientRequest()
        msg.type = ClientRequest.STATS
        self._send_message(msg)
        msg = self.connection.receive_message()
        cmsg = ClientResponse()
        cmsg.ParseFromString(msg)
        assert cmsg.type == ClientResponse.STATS
        return {
            "n_workers": cmsg.stats.n_workers,
            "n_data_objects": cmsg.stats.n_data_objects
        }

    def close(self):
        self.connection.close()

    def terminate(self):
        """ Terminate server & workers """
        msg = ClientRequest()
        msg.type = ClientRequest.TERMINATE
        self._send_message(msg)

    def set_trace(self, trace_path):
        self.trace_path = trace_path
        msg = ClientRequest()
        msg.type = ClientRequest.TRACE
        msg.trace_path = os.path.abspath(trace_path)
        self._send_message(msg)

    def wait_one(self, future):
        if future.finished():
            return
        self._process_events(lambda f: f == future)

    def wait(self, futures):
        for f in futures:
            f.wait()

    def fetch_one(self, future):
        def set_data(data_id, data):
            assert future.task_id == data_id
            future.set_result(data)
            return True

        if future.has_result:
            return future._result

        status = future.remote_status
        if status == "running":
            self.wait_one(future)
        elif status == "finished":
            pass
        else:
            raise Exception("Fetch called on invalid future")

        self.wait_one(future)
        msg = ClientRequest()
        msg.type = ClientRequest.FETCH
        msg.id = future.task_id
        self._send_message(msg)
        self._process_events(on_data=set_data)
        return future._result

    def gather_one(self, future):
        result = self.fetch_one(future)
        future.release()
        return result

    def fetch(self, futures):
        results = {}
        for f in self.as_completed(futures):
            results[f] = self.fetch_one(f)
        return [results[f] for f in futures]

    def gather(self, futures):
        results = {}
        for f in self.as_completed(futures):
            results[f] = self.fetch_one(f)
            f.release()
        return [results[f] for f in futures]

    def as_completed(self, futures):
        futures = set(futures)
        n_finished_tasks = 0
        while futures:
            if n_finished_tasks != self.n_finished_tasks:
                for f in futures:
                    status = f.remote_status
                    if status == "running":
                        pass
                    elif status == "finished":
                        futures.remove(f)
                        yield f
                        break
                    else:
                        raise Exception("Fetch called on invalid future")
                else:
                    n_finished_tasks = self.n_finished_tasks
                continue
            f = self._process_events(lambda f: f in futures)
            futures.remove(f)
            n_finished_tasks += 1
            yield f

    def release(self, futures):
        for f in futures:
            self.release_one(f)

    def release_one(self, future):
        status = future.remote_status
        if status == "finished":
            msg = ClientRequest()
            msg.type = ClientRequest.RELEASE
            msg.id = future.task_id
            self._send_message(msg)
            future.remote_status = "released"
        elif status == "released" or status == "canceled":
            pass  # Do nothing, task is already released
        elif status == "running":
            future.remote_status = "canceled"
        else:
            raise Exception("Unknown status")

    def _process_events(self, on_finished=None, on_data=None):
        while True:
            msg = self.connection.receive_message()
            cmsg = ClientResponse()
            cmsg.ParseFromString(msg)
            t = cmsg.type
            if t == ClientResponse.TASK_FINISHED:
                self.n_finished_tasks += 1
                future = self.futures.pop(cmsg.id)
                future.set_finished()
                if on_finished and on_finished(future):
                    return future
            elif t == ClientResponse.DATA:
                data = self._receive_data(cmsg.data.type_id)
                if on_data(cmsg.data.id, data):
                    return data
            elif t == ClientResponse.TASK_FAILED:
                self.process_task_failed(cmsg)
            elif t == ClientResponse.ERROR:
                self.process_error(cmsg)
            else:
                print(t)
                assert 0

    def submit_one(self, task):
        return self.submit((task,))[0]

    def submit(self, tasks):
        """Submits tasks to the server and ...

        Args:
            tasks ([Task]): Tasks that are submitted

        Example:
            >>> from loom.client import Client, tasks
            >>> client = Client("server", 9010)
            >>> task1 = tasks.const("Hello")
            >>> task2 = tasks.const("World")
            >>> result = client.submit((task1, task2))
            >>> print(client.gather(result))
        """

        id_base = self.submit_id

        plan = Plan(id_base)
        futures = self.futures
        results = []
        for task in tasks:
            if not isinstance(task, Task):
                raise Exception("{} is not a task".format(task))
            plan.add(task)
            task_id = plan.tasks[task]
            future = futures.get(task_id)
            if future is None:
                future = Future(self, task_id)
                futures[task_id] = future
            results.append(future)

        self.submit_id += plan.id_counter

        msg = ClientRequest()
        msg.type = ClientRequest.PLAN

        include_metadata = self.trace_path is not None
        msg.plan.id_base = id_base
        plan.set_message(
            msg.plan, self.symbols,
            frozenset(tasks), include_metadata)

        self._send_message(msg)
        return results

    def _symbol_list(self):
        symbols = [None] * len(self.symbols)
        for name, index in self.symbols.items():
            symbols[index] = name
        return symbols

    def _read_symbols(self):
        msg = self.connection.receive_message()
        cmsg = ClientResponse()
        cmsg.ParseFromString(msg)
        assert cmsg.type == ClientResponse.DICTIONARY
        self.symbols = {}
        for i, s in enumerate(cmsg.symbols):
            self.symbols[s] = i
        self.array_id = self.symbols.get("loom/array")
        self.rawdata_id = self.symbols.get("loom/data")
        self.pyobj_id = self.symbols.get("loom/pyobj")

    def process_task_failed(self, cmsg):
        assert cmsg.HasField("error")
        error = cmsg.error
        raise TaskFailed(error.id, error.worker, error.error_msg)

    def process_error(self, cmsg):
        assert cmsg.HasField("error")
        error = cmsg.error
        raise LoomError(error.error_msg)

    def _receive_data(self, type_id):
        if type_id == self.rawdata_id:
            return self.connection.receive_message()
        if type_id == self.array_id:
            types = self.connection.receive_message()
            assert len(types) % 4 == 0
            result = []
            for i in range(0, len(types), 4):
                type_id = struct.unpack_from("I", types, i)[0]
                result.append(self._receive_data(type_id))
            return result
        if type_id == self.pyobj_id:
            return cloudpickle.loads(self.connection.receive_message())
        assert 0

    def _send_message(self, message):
        data = message.SerializeToString()
        self.connection.send_message(data)


def make_dry_report(tasks, report_filename):
    """Creates a report without submitting to the server

    Args:
         tasks (Task or [Task]): Tasks for which the report is composed
         report_filename (str): Filename of the resulting file
    """
    raise Exception("Not implemented in this version")
