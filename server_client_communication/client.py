import socket
import time
from helpers import current_millis, generate_mock_data


# Server IP and port
SERVER_IP = "192.168.1.100"  # Use your actual server IP if remote
PORT = 3333

# Markers and payload
START = "SENDING_DATA\n"
END = "DATA_SENT"
DATA = generate_mock_data(rows=3)

# Create socket and connect
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((SERVER_IP, PORT))

print("Connected to server.")

print("Sending data...")
sock.sendall(START.encode('utf-8'))
time.sleep(0.05)
sock.sendall(DATA.encode('utf-8'))
time.sleep(0.05)
sock.sendall(END.encode('utf-8'))
print("Data sent")

response = sock.recv(1024).decode('utf-8').strip()
print(f"Wakeup time from server: {response}")

input("Press Enter to disconnect...")
sock.close()