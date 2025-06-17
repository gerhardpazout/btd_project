import socket
import time

SERVER_IP = "192.168.1.100"  # Use your actual server IP if remote
PORT = 3333

# Create socket and connect
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((SERVER_IP, PORT))

print("Connected to server.")

input("Press Enter to disconnect...")
sock.close()