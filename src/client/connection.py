
import struct

u32 = struct.Struct("<I")


class Connection(object):

    def __init__(self, socket):
        self.socket = socket
        self.data = ""

    def receive_message(self):
        while True:
            size = len(self.data)
            if size > 4:
                msg_size = u32.unpack(self.data[:4])[0]
                msg_size += 4
                if size >= msg_size:
                    message = self.data[4:msg_size]
                    self.data = self.data[msg_size:]
                    return message
            new_data = self.socket.recv(65536)
            if not new_data:
                raise Exception("Connection to server lost")
            self.data += new_data

    def read_data(self, data_size):
        result = ""
        while True:
            change = min(data_size, len(self.data))
            result += self.data[:change]
            self.data = self.data[change:]
            data_size -= change
            if data_size == 0:
                return result
            self.data = self.socket.recv(65536)

    def send_message(self, data):
        data = u32.pack(len(data)) + data
        self.socket.sendall(data)
