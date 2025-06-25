import socket
import time
from helpers import generate_mock_data, generate_wakeup_window, send_data_to_server

"""
This file is just a way of simulating the ESP, so we can develop and quality control the server.py faster
"""

# Constants
SERVER_IP = "192.168.1.100"
PORT = 3333

START = "SENDING_DATA\n"
END   = "DATA_SENT\n"
WAKE_UP_WINDOW_MARKER = "WAKE_WINDOW"

# Simulate wakup window
wakeup_window = generate_wakeup_window()
print(wakeup_window)

# Debug generate_mock_data method
print(generate_mock_data(3))

# Connect to server
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((SERVER_IP, PORT))
print("Connected to server.")

# Send wakup window to server
msg = f"WAKE_WINDOW,{wakeup_window[0]},{wakeup_window[1]}\n"
sock.sendall(msg.encode('utf-8'))

response = sock.recv(1024).decode().strip()
print(f"Server response to wake window: {response}")
sock.close()
time.sleep(1)

# send data to server
send_data_to_server(SERVER_IP, PORT, START, END, 2)
