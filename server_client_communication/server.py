import socket
import csv
import time
from pathlib import Path

import socket

HOST = socket.gethostbyname(socket.gethostname())  # or "0.0.0.0"
PORT = 3333

# Create TCP socket
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen(1)

print(f"Server listening on {HOST}:{PORT}")
print("Waiting for a client to connect...")

# Wait and accept a client
conn, addr = server.accept()
print(f"Client connected from {addr[0]}:{addr[1]}")

# Just keep the connection open
input("Press Enter to close connection...")

conn.close()
server.close()
print("Server closed.")