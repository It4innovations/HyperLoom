from loomcomm_pb2 import Register, Data, ClientMessage

import socket
from connection import Connection
from plan import Task

LOOM_PROTOCOL_VERSION = 1


class Client(object):

    def __init__(self, address, port, info=False):
        self.server_address = address
        self.server_port = port
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((address, port))
        self.connection = Connection(s)

        if info:
            self.info = []
        else:
            self.info = None

        msg = Register()
        msg.type = Register.REGISTER_CLIENT
        msg.protocol_version = LOOM_PROTOCOL_VERSION
        msg.info = info
        self._send_message(msg)

    def submit(self, plan, results):

        msg = plan.create_message()

        if isinstance(results, Task):
            single_result = True
            msg.result_ids.extend((results.id,))
            expected = 1
        else:
            single_result = False
            r = set(results)
            msg.result_ids.extend(r.id for r in r)
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
            else:
                assert cmsg.type == ClientMessage.INFO
                self.add_info(cmsg.info)

        if single_result:
            return data[results.id]
        else:
            return [data[task.id] for task in results]

    def add_info(self, info):
        self.info.append((info.id, info.worker))

    def _receive_data(self):
        msg_data = Data()
        msg_data.ParseFromString(self.connection.receive_message())
        type_id = msg_data.type_id
        if type_id == 300:  # Data
            return self.connection.read_data(msg_data.arg0_u64)
        if type_id == 400:  # Array
            return [self._receive_data() for i in xrange(msg_data.arg0_u64)]
        assert 0

    def _send_message(self, message):
        data = message.SerializeToString()
        self.connection.send_message(data)
