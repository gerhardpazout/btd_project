import socket
import csv
import time
from pathlib import Path
from helpers import chunker, save_to_csv, calculate_wakeup_time

HOST = socket.gethostbyname(socket.gethostname())  # or "0.0.0.0"
PORT = 3333

START_MARKER = "SENDING_DATA"
END_MARKER = "DATA_SENT"

# CSV related
CSV_PATH = Path("values.csv")
COLUMNS = ["x", "y", "z"]  # Change if needed

# Create TCP socket
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen(1)

print(f"Server listening on {HOST}:{PORT}")
print("Waiting for a client to connect...")

# Wait and accept a client
conn, addr = server.accept()
print(f"Client connected from {addr[0]}:{addr[1]}")

buffer = ""
msg = ""
receiving = False

while True:
    data = conn.recv(1024)
    if not data:
        print("Client disconnected unexpectedly")
        break

    decoded = data.decode('utf-8')
    buffer += decoded

    if not receiving:
        if START_MARKER in buffer:
            receiving = True
            buffer = buffer.split(START_MARKER, 1)[1]  # remove marker
        else:
            continue

    if END_MARKER in buffer:
        msg += buffer.split(END_MARKER, 1)[0]
        break
    else:
        msg += buffer
        buffer = ""

# Print the received message
print("Received data:")
print(msg)

# parse values
values = [float(x) for x in msg.split(",") if x.strip()]
rows = list(chunker(values, len(COLUMNS)))

# Save data to CSV
save_to_csv(rows, path=CSV_PATH, columns=COLUMNS)

print(f"Saved {len(rows)} rows to {CSV_PATH}")

# Call calculation and send result
wakeup_time = calculate_wakeup_time()
conn.sendall(wakeup_time.encode('utf-8'))
print(f"Sent wakeup time: {wakeup_time}")

# Just keep the connection open
input("Press Enter to close connection...")

conn.close()
server.close()
print("Server closed.")