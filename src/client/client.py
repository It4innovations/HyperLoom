from loomcomm_pb2 import Register, Data, ClientMessage, ClientSubmit
from loomreport_pb2 import Report

import socket
from connection import Connection
from plan import Task

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

    def submit(self, plan, results, report=None):
        msg = ClientSubmit()
        msg.report = bool(report)
        plan.set_message(msg.plan, self.symbols)

        if isinstance(results, Task):
            single_result = True
            msg.plan.result_ids.extend((results.id,))
            expected = 1
        else:
            single_result = False
            r = set(results)
            msg.plan.result_ids.extend(r.id for r in r)
            expected = len(r)

        self._send_message(msg)

        data = {}
        while expected != len(data):
            msg = self.connection.receive_message()
            cmsg = ClientMessage()
            cmsg.ParseFromString(msg)
            if cmsg.type == ClientMessage.DATA:
                prologue = cmsg.data
                data[prologue.id] = self._receive_data()
            elif cmsg.type == ClientMessage.EVENT:
                self.process_event(cmsg.event)
            elif cmsg.type == ClientMessage.ERROR:
                self.process_error(cmsg)
            else:
                assert 0

        if report:
            self._write_report(report, plan)

        if single_result:
            return data[results.id]
        else:
            return [data[task.id] for task in results]

    def _symbol_list(self):
        symbols = [None] * len(self.symbols)
        for name, index in self.symbols.items():
            symbols[index] = name
        return symbols

    def _write_report(self, report_filename, plan):
        report_msg = Report()
        report_msg.symbols.extend(self._symbol_list())
        plan.set_message(report_msg.plan, self.symbols)

        with open(report_filename + ".report", "w") as f:
            f.write(report_msg.SerializeToString())

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

    def process_event(self, event):
        assert 0

    def _receive_data(self):
        msg_data = Data()
        msg_data.ParseFromString(self.connection.receive_message())
        type_id = msg_data.type_id
        if type_id == self.rawdata_id:
            return self.connection.read_data(msg_data.size)
        if type_id == self.array_id:
            return [self._receive_data() for i in xrange(msg_data.length)]
        print type_id, self.array_id, self.rawdata_id
        assert 0

    def _send_message(self, message):
        data = message.SerializeToString()
        self.connection.send_message(data)
