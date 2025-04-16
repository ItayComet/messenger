# MessageU - End-to-End Encrypted Messaging App

This project implements a secure client-server messaging application with end-to-end encryption, designed as part of an academic assignment. The server is implemented in Python, while the client is developed in C++.
The project was an assignment for "Defensive Programming" course, and for that reason has some vulnerabilities in the protocol. These vulnerabilities will be fixed in the future.

## Features

- **End-to-End Encryption**: Messages are encrypted and decrypted only on the clients. The server cannot view message contents.
- **Asynchronous Communication**: Clients can send messages even if the recipient is offline.
- **Stateless Server**: Each request is self-contained. Server maintains no state between requests.
- **Client Registration**: Clients register with a public key and recieve a unique UUID.
- **Secure Messaging**: Includes symmetric and asymmetric encryption for secure communication.
- **Persistent Storage**: Server can store user/message data in an SQLite database.

## Architecture Overview

- **Client**: C++ console application using:
  - `Boost` for TCP networking
  - `Crypto++` for all cryptographic operations (AES, RSA, Base64)
  - Reads server info from `server.info` and user credentials from `me.info`

- **Server**: Python application using:
  - `selectors` to support multiple simultaneous clients
  - Stateless by default – handles each request independently
  - SQLite database (`defensive.db`) to store clients and messages persistently

- **Protocol**: Custom binary protocol over TCP
  - All numeric fields are unsigned and little-endian
  - Requests and responses have structured headers and payloads

- **Encryption**:
  - Full **end-to-end encryption**: only clients can encrypt/decrypt messages
  - RSA used for exchanging symmetric keys
  - AES used for encrypting message contents between clients
 
### Dependencies
- Boost (networking)
- Crypto++ (encryption)
