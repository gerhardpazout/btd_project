import socket
import csv
import time
from pathlib import Path

import socket

HOST = socket.gethostbyname(socket.gethostname())  # or "0.0.0.0"
PORT = 3333

START_MARKER = "SENDING_DATA"
END_MARKER = "DATA_SENT"

# CSV related
CSV_PATH = Path("values.csv")
COLUMNS = ["x", "y", "z"]  # Change if needed

def chunker(seq, size):
    return (seq[pos:pos + size] for pos in range(0, len(seq), size))

# calculate wakeup time
def calculate_wakeup_time():
    print("Calculating wakeup time from CSV...")
    if not CSV_PATH.exists():
        print("CSV file not found.")
        return "00:00"  # fallback

    with open(CSV_PATH, newline="") as f:
        reader = csv.reader(f)
        header = next(reader)  # skip column names
        first_row = next(reader, None)

    if first_row:
        try:
            values = [float(x) for x in first_row]
            avg = sum(values) / len(values)
            print(f"First row: {values} → avg: {avg:.2f}")
        except Exception as e:
            print(f"Failed to parse row: {e}")
    else:
        print("CSV is empty")

    return "07:00"  # hardcoded wake-up time

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

# Save to CSV
with open(CSV_PATH, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(COLUMNS)
    for row in rows:
        writer.writerow(row)

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