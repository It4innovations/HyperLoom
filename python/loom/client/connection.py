
import struct

u64 = struct.Struct("<Q")


class Connection(object):

    def __init__(self, socket):
        self.socket = socket
        self.data = bytes()

    def close(self):
        self.socket.close()

    def receive_message(self):
        while True:
            size = len(self.data)
            if size >= 8:
                msg_size = u64.unpack(self.data[:8])[0]
                msg_size += 8
                if size >= msg_size:
                    message = self.data[8:msg_size]
                    self.data = self.data[msg_size:]
                    return message
            new_data = self.socket.recv(65536)
            if not new_data:
                raise Exception("Connection to server lost")
            self.data += new_data

    def read_data(self, data_size):
        result = bytes()
        while True:
            change = min(data_size, len(self.data))
            result += self.data[:change]
            self.data = self.data[change:]
            data_size -= change
            if data_size == 0:
                return result
            self.data = self.socket.recv(65536)

    def send_message(self, data):
        data = u64.pack(len(data)) + data
        self.socket.sendall(data)
