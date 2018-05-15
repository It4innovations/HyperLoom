
import struct

u64 = struct.Struct("<Q")


class Connection(object):

    def __init__(self, socket):
        self.socket = socket
        self.data = bytes()

    def close(self):
        self.socket.close()

    def receive_message(self):
        while len(self.data) < 8:
            self.data += self.socket.recv(655360)
        msg_size = u64.unpack(self.data[:8])[0]
        if len(self.data) >= msg_size + 8:
            msg_size = msg_size + 8
            message = self.data[8:msg_size]
            self.data = self.data[msg_size:]
            return message
        self.data = self.data[8:]
        return self.read_data(msg_size)

    def read_data(self, data_size):
        chunks = []
        while True:
            if data_size >= len(self.data):
                chunks.append(self.data)
                data_size -= len(self.data)
                if data_size == 0:
                    self.data = b""
                    return b"".join(chunks)
            else:
                chunks.append(self.data[:data_size])
                self.data = self.data[data_size:]
                return b"".join(chunks)
            self.data = self.socket.recv(655360)
            if not self.data:
                raise Exception("Connection to server lost")

    def send_message(self, data):
        data = u64.pack(len(data)) + data
        self.socket.sendall(data)
