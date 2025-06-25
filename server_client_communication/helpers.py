import socket
import csv
import time
import random
from pathlib import Path
from datetime import datetime

"""
This file contains all the helper functions used for the server-client communication
"""

CSV_PATH = Path("values.csv")

# chunker
def chunker(seq, size):
    return (seq[pos:pos + size] for pos in range(0, len(seq), size))

# Simulated sample data with timestamps (ms)
def current_millis():
    return int(time.time() * 1000)

# save data to CSV
def save_to_csv(rows, path=CSV_PATH, columns=None):
    print(f"Saving {len(rows)} rows to {path}")
    with open(path, "w", newline="") as f:
        writer = csv.writer(f)
        if columns:
            writer.writerow(columns)
        for row in rows:
            writer.writerow(row)


# calculate wakeup time
def calculate_wakeup_time(csv_path=CSV_PATH):
    print("Calculating wakeup time from CSV...")
    if not csv_path.exists():
        print("CSV file not found.")
        return "00:00"  # fallback

    with open(csv_path, newline="") as f:
        reader = csv.reader(f)
        header = next(reader)  # skip column names
        first_row = next(reader, None)

    if first_row:
        try:
            values = [float(x) for x in first_row[1:]]
            avg = sum(values) / len(values)
            print(f"First row: {values} → avg: {avg:.2f}")
        except Exception as e:
            print(f"Failed to parse row: {e}")
    else:
        print("CSV is empty")

    return "07:00"  # hardcoded wake-up time

def generate_mock_data(n_samples=2, delay_ms=0):
    """
    Generates mock CSV-style data with one sample per line.
    Returns a newline-separated string.
    """
    rows = []
    for _ in range(n_samples):
        ts = int(time.time() * 1000)
        x = round(random.uniform(-1, 1), 2)
        y = round(random.uniform(-1, 1), 2)
        z = round(random.uniform(-1, 1), 2)
        temp = round(random.uniform(20.0, 40.0), 2)
        rows.append(f"{ts},{x},{y},{z},{temp}")
        time.sleep(delay_ms / 1000.0)
    return "\n".join(rows) + "\n"

def generate_wakeup_window():
    wakeup_window = [0, 0]
    now_ms = int(time.time() * 1000)
    wake_start = now_ms + 10_000 # 10s from now
    wake_end   = now_ms + 60_000 # 30s from now
    wakeup_window[0] = wake_start
    wakeup_window[1] = wake_end
    return wakeup_window

def send_data_to_server(server_ip, port, marker_start="SENDING_DATA\n", marker_end="DATA_SENT\n", times=1):
    for i in range (times):
        DATA = generate_mock_data(n_samples=300)

        # Upload file contents with start/end markers
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((server_ip, port))
        sock.sendall(marker_start.encode('utf-8'))
        sock.sendall(DATA.encode('utf-8'))
        sock.sendall(marker_end.encode('utf-8'))
        print("Data sent.")

        # Wait for ACK from server (optional)
        response = sock.recv(1024).decode('utf-8').strip()
        print(f"Server response: {response}")

        sock.close()
        time.sleep(1)
        print("Connection closed.")

def ts_to_hhmmss(timestamp_ms):
    """Convert a Unix timestamp in milliseconds to HH:MM format (24h clock)."""
    dt = datetime.fromtimestamp(timestamp_ms / 1000)
    return dt.strftime("%H:%M:%S")