import struct
from enum import Enum
import uuid
from database import Database
from utils import create_response, ResponseCode

UUID_SIZE = 16
VERSION_SIZE = 1
CODE_SIZE = 2
PAYLOAD_SIZE_SIZE = 4
REQ_HEADER_SIZE = UUID_SIZE + VERSION_SIZE + CODE_SIZE + PAYLOAD_SIZE_SIZE
RESPONSE_HEADER_SIZE = VERSION_SIZE + CODE_SIZE + PAYLOAD_SIZE_SIZE
USER_NAME_ARR_SIZE = 255
PUBLIC_KEY_SIZE = 160


class RequestCode(Enum):
    REGISTER = 600
    USER_LIST = 601
    PUBLIC_KEY = 602
    SEND_MESSAGE = 603
    PULL_MESSAGES = 604


# Handles a request based on the code.
# the version argument will be the version of the host and not the client.
def process_request(version, code, payload, client_id):
    try:
        with Database() as db:
            db.update_last_seen(client_id)
            if code == RequestCode.REGISTER.value:
                if len(payload) < USER_NAME_ARR_SIZE + PUBLIC_KEY_SIZE:  # invalid payload
                    print("error return")
                    return create_response(version, ResponseCode.ERROR.value, b'')  # Invalid payload
                name, public_key = struct.unpack(f'{USER_NAME_ARR_SIZE}s {PUBLIC_KEY_SIZE}s', payload)
                client_id = uuid.uuid4().bytes  # Generate a new random UUID
                return db.register_user(version, client_id, name.decode().strip('\x00'), public_key)
            elif code == RequestCode.USER_LIST.value:
                return db.get_users(version, client_id)
            elif code == RequestCode.PUBLIC_KEY.value:
                if len(payload) < UUID_SIZE:
                    return create_response(version, ResponseCode.ERROR.value, b'')  # Invalid client ID request
                return db.get_public_key(version, payload[:UUID_SIZE])
            elif code == RequestCode.SEND_MESSAGE.value:
                if len(payload) < 21: # Message header size
                    return create_response(version, ResponseCode.ERROR.value, b'')  # Invalid message request
                return db.store_message(version, payload, client_id)
            elif code == RequestCode.PULL_MESSAGES.value:
                return db.get_pending_messages(version, client_id)
    except Exception as e:
        print(f"Error processing request: {e}")
    return create_response(version, ResponseCode.ERROR.value, b'')  # General error response
