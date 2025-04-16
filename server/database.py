import sqlite3
import struct
from utils import create_response, ResponseCode
DATABASE_NAME = "defensive.db"


# This class uses sql prompts to create and handle a database file for the host
# It then returns a response ready to be sent to a client, based on the request
class Database:
    @staticmethod
    def initialize():
        conn = sqlite3.connect(DATABASE_NAME)
        cursor = conn.cursor()
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS clients (
                ID BLOB CHECK(length(ID) = 16) PRIMARY KEY,
                UserName char(255) UNIQUE,
                PublicKey BLOB,
                LastSeen TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )""")
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS messages (
                ID INTEGER PRIMARY KEY AUTOINCREMENT,
                ToClient BLOB CHECK(length(ToClient) = 16),
                FromClient BLOB CHECK(length(FromClient) = 16),
                Type INTEGER,
                Content BLOB
            )""")
        conn.commit()
        cursor.close()
        conn.close()

    def __enter__(self):
        self.conn = sqlite3.connect(DATABASE_NAME)
        self.cursor = self.conn.cursor()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.conn.commit()
        self.cursor.close()
        self.conn.close()

    # updates the last seen field of a user to the current time
    def update_last_seen(self, client_id):
        self.cursor.execute("SELECT 1 FROM clients WHERE ID=?", (client_id,))
        if self.cursor.fetchone():
            self.cursor.execute("UPDATE clients SET LastSeen=CURRENT_TIMESTAMP WHERE ID=?", (client_id,))
            self.conn.commit()

    # Registers a user in the database, then returns a packet with the client id generated for them
    def register_user(self, version, client_id, name, public_key):
        try:
            self.cursor.execute(
                "INSERT INTO clients (ID, UserName, PublicKey, LastSeen) VALUES (?, ?, ?, CURRENT_TIMESTAMP)",
                (client_id, name, public_key))
            return create_response(version, ResponseCode.REGISTRATION_SUCCESS.value, client_id)
        except sqlite3.IntegrityError:
            return create_response(version, ResponseCode.ERROR.value, b'')  # Username already exists error

    # Returns a user list, each user has a user name and uuid
    def get_users(self, version, requesting_client_id):
        self.cursor.execute("SELECT ID, UserName FROM clients WHERE ID != ?", (requesting_client_id,))
        users = self.cursor.fetchall()
        user_list = b''.join([struct.pack('<16s 255s', row[0], row[1].encode().ljust(255, b'\x00')) for row in users])
        return create_response(version, ResponseCode.USER_LIST.value, user_list)

    # returns a response with the client id and the public key of the client id
    def get_public_key(self, version, client_id):
        self.cursor.execute("SELECT PublicKey FROM clients WHERE ID=?", (client_id,))
        result = self.cursor.fetchone()
        if result:
            return create_response(version, ResponseCode.PUBLIC_KEY.value, client_id + result[0])
        return create_response(version, ResponseCode.ERROR.value, b'')  # Error code

    # Stores messages from a client to another client in the db
    def store_message(self, version, payload, from_id):
        to_id, msg_type, content_size = struct.unpack('<16s B I', payload[:21]) # extract message header
        content = payload[21:]
        self.cursor.execute("INSERT INTO messages (ToClient, FromClient, Type, Content) VALUES (?, ?, ?, ?)",
                            (to_id, from_id, msg_type, content))
        return create_response(version, ResponseCode.MESSAGE_SENT.value, from_id)

    # Gets all messages stored in the db files ini which client_id is the receiver
    # and then packs them with the headers as written in the protocol.
    # The packet which will be sent consists of one main header for the response
    # and a set of message header+content combinations
    def get_pending_messages(self, version, client_id):
        # pull messages where the receiver is client_id
        self.cursor.execute(
            "SELECT ID, FromClient, Type, Content FROM messages WHERE ToClient = ?",
            (client_id,)
        )
        messages = self.cursor.fetchall()

        payload = b''
        for message_id, from_client, msg_type, content in messages:
            message_header = struct.pack(
                '<16s I B I',
                from_client,
                message_id,
                msg_type,
                len(content)
            )
            payload += message_header + content

        # Delete delivered messages
        self.cursor.execute("DELETE FROM messages WHERE ToClient = ?", (client_id,))

        return create_response(version, ResponseCode.RETURN_MESSAGES.value, payload)
