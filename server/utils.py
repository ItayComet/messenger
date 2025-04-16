import struct
from enum import Enum

PORT_FILE = "myport.info"
DEFAULT_PORT = 1357


class ResponseCode(Enum):
    REGISTRATION_SUCCESS = 2100
    USER_LIST = 2101
    PUBLIC_KEY = 2102
    MESSAGE_SENT = 2103
    RETURN_MESSAGES = 2104
    ERROR = 9000


# Returns, if valid and exists, the port as provided in the first 5 characters in
# the file port file. If the file doesn't exists, or the first 5 characters in the file
# are not a valid port, returns the default port
def get_port():
    try:
        with open(PORT_FILE, "r") as f:
            port = f.read(5)  # a port can't be longer than 5 characters in decimal notation
            if not port.isdigit():
                print(f"Port provided is not a number, using default {DEFAULT_PORT} port.")
                return DEFAULT_PORT
            else:
                if int(port) > 65535: # port is longer than 16 bits
                    print(f"Port provided is not valid, using default {DEFAULT_PORT} port")
                    return DEFAULT_PORT
                print("Starting with port " + str(int(port)) + ".")
                return int(port)
    except IOError:
        print(f"The file \"{PORT_FILE}\" does not exist, using default {DEFAULT_PORT} port.")
        return DEFAULT_PORT


# Creates a response ready to be sent to a client, based on the arguments
def create_response(version, code, payload):
    payload_size = len(payload)
    header = struct.pack('<B H I', version, code, payload_size)
    return header + payload
