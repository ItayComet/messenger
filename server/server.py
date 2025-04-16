import selectors
import protocol
import socket
import struct
from protocol import process_request
from database import Database
from utils import create_response

VERSION = 2     # since the server uses a database
MAX_QUEUE = 100 # A max number of queued connections

# A server object will be initialised with host IP and port, and then after setting itself up,
# it will accept connections and requests from clients using selectors class.
class Server:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.selector = selectors.DefaultSelector()
        self.sock = socket.socket()
        Database.initialize()

    def start(self):
        # Begin listening and handling requests
        try:
            self.sock.bind((self.host, self.port))
            self.sock.listen(MAX_QUEUE)
            self.sock.setblocking(False)
            self.selector.register(self.sock, selectors.EVENT_READ, self.accept)
        except Exception as e:
            print(f"Error setting up server socket: {e}")
            return False
        print("Server is now listening for connections:")
        # Message handling loop
        while True:
            try:
                events = self.selector.select()
                for key, mask in events:
                    callback = key.data
                    callback(key.fileobj)
            except Exception as e:
                print(f"Error in event loop: {e}")

    def accept(self, sock):
        conn, addr = sock.accept()
        print(f"Accepted connection from {addr}")
        conn.setblocking(False)
        self.selector.register(conn, selectors.EVENT_READ, self.handle_request)

    # this function will be called when the server reads a new request from a client
    # Here the header will be parsed and then will be sent to the protocol function
    # together with the payload to be processed.
    def handle_request(self, conn):
        try:
            header = conn.recv(protocol.REQ_HEADER_SIZE)
            if header == b'':   # client disconnected
                self.selector.unregister(conn)
                conn.close()
                return
            if not header or len(header) < protocol.REQ_HEADER_SIZE:
                error_response = create_response(VERSION, protocol.ResponseCode.ERROR, b'')
                conn.send(error_response)
                return
            client_id, version, code, payload_size = struct.unpack('<16s B H I', header)
            payload = conn.recv(payload_size) if payload_size > 0 else b''
            response = process_request(VERSION, code, payload, client_id)
            conn.send(response)
            print(f"Response sent successfully.")
        except (BrokenPipeError, ConnectionResetError):
            print("Client forcibly closed the connection.")
            self.selector.unregister(conn)
            conn.close()
        except Exception as e:
            print(f"Error handling client: {e}")
            error_response = create_response(VERSION, protocol.ResponseCode.ERROR, b'')
            try:
                conn.send(error_response)
            except Exception as e0:
                print(f"Failed to send error response to client: {e0}")
            finally:
                self.selector.unregister(conn)
                conn.close()
