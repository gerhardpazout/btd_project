import socket
from helpers import generate_mock_data

SERVER_IP = "192.168.1.100"
PORT = 3333

START = "SENDING_DATA\n"
END   = "DATA_SENT\n"

# Simulate a large buffered upload (e.g., 10 seconds @ 30Hz = 300 samples)
DATA = generate_mock_data(n_samples=300)  # timestamp,x,y,z,temp repeated 300×

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((SERVER_IP, PORT))
print("Connected to server.")

# Upload file contents with start/end markers
sock.sendall(START.encode('utf-8'))
sock.sendall(DATA.encode('utf-8'))
sock.sendall(END.encode('utf-8'))
print("Data sent.")

# Wait for ACK from server (optional)
response = sock.recv(1024).decode('utf-8').strip()
print(f"Server response: {response}")

sock.close()
print("Connection closed.")
