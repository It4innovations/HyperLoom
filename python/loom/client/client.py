
from .connection import Connection
from .task import Task
from .plan import Plan

from ..pb.loomcomm_pb2 import Register, Data, ClientMessage, ClientSubmit
from ..pb.loomreport_pb2 import Report

import socket

LOOM_PROTOCOL_VERSION = 1


class LoomException(Exception):
    pass


class TaskFailed(LoomException):

    def __init__(self, id, worker, error_msg):
        self.id = id
        self.worker = worker
        self.error_msg = error_msg
        message = "Task id={} failed: {}".format(id, error_msg)
        LoomException.__init__(self, message)


class Client(object):

    def __init__(self, address, port):
        self.server_address = address
        self.server_port = port

        self.symbols = None

        self.array_id = None
        self.rawdata_id = None

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((address, port))
        self.connection = Connection(s)

        msg = Register()
        msg.type = Register.REGISTER_CLIENT
        msg.protocol_version = LOOM_PROTOCOL_VERSION
        self._send_message(msg)

        while self.symbols is None:
            self._read_symbols()

    def submit(self, tasks, report=None):
        if isinstance(tasks, Task):
            single_result = True
            tasks = (tasks,)
        else:
            single_result = False

        task_set = set(tasks)

        plan = Plan()
        for task in task_set:
            plan.add(task)

        msg = ClientSubmit()
        msg.report = bool(report)

        msg.plan.result_ids.extend(plan.tasks[t] for t in task_set)
        expected = len(task_set)
        plan.set_message(msg.plan, self.symbols)

        self._send_message(msg)

        if report:
            report_data = self._create_report(plan)

        data = {}
        while expected != len(data):
            msg = self.connection.receive_message()
            cmsg = ClientMessage()
            cmsg.ParseFromString(msg)
            if cmsg.type == ClientMessage.DATA:
                prologue = cmsg.data
                data[prologue.id] = self._receive_data()
            elif cmsg.type == ClientMessage.EVENT:
                self.process_event(cmsg.event, report_data)
            elif cmsg.type == ClientMessage.ERROR:
                self.process_error(cmsg)
            else:
                assert 0

        if report:
            write_report(report_data, report)

        if single_result:
            return data[plan.tasks[tasks[0]]]
        else:
            return [data[plan.tasks[t]] for t in tasks]

    def _symbol_list(self):
        symbols = [None] * len(self.symbols)
        for name, index in self.symbols.items():
            symbols[index] = name
        return symbols

    def _create_report(self, plan):
        report_msg = Report()
        report_msg.symbols.extend(self._symbol_list())
        plan.set_message(report_msg.plan, self.symbols)
        return report_msg

    def _read_symbols(self):
        msg = self.connection.receive_message()
        cmsg = ClientMessage()
        cmsg.ParseFromString(msg)
        assert cmsg.type == ClientMessage.DICTIONARY
        self.symbols = {}
        for i, s in enumerate(cmsg.symbols):
            self.symbols[s] = i
        self.array_id = self.symbols["loom/array"]
        self.rawdata_id = self.symbols["loom/data"]

    def process_error(self, cmsg):
        assert cmsg.HasField("error")
        error = cmsg.error
        raise TaskFailed(error.id, error.worker, error.error_msg)

    def process_event(self, event, report_data):
        new_event = report_data.events.add()
        new_event.CopyFrom(event)

    def _receive_data(self):
        msg_data = Data()
        msg_data.ParseFromString(self.connection.receive_message())
        type_id = msg_data.type_id
        if type_id == self.rawdata_id:
            return self.connection.read_data(msg_data.size)
        if type_id == self.array_id:
            return [self._receive_data() for i in range(msg_data.length)]
        print(type_id, self.array_id, self.rawdata_id)
        assert 0

    def _send_message(self, message):
        data = message.SerializeToString()
        self.connection.send_message(data)


def make_dry_report(tasks, report_filename):
    if isinstance(tasks, Task):
        tasks = (tasks,)

    plan = Plan()
    for task in tasks:
        plan.add(task)

    # Create symbols
    symbols = sorted(plan.collect_symbols())
    symbol_table = {}
    for i, s in enumerate(symbols):
        symbol_table[s] = i

    # Create report
    report_data = Report()
    report_data.symbols.extend(symbols)
    plan.set_message(report_data.plan, symbol_table)

    write_report(report_data, report_filename)


def write_report(report_data, report_filename):
    with open(report_filename, "wb") as f:
        f.write(report_data.SerializeToString())
