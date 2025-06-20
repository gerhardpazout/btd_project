import socket
import csv
import time
import random
from pathlib import Path

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
    Generates mock data in the format:
    [timestamp, x, y, z, temp, ...]
    Returns a comma-separated string.
    """
    import random, time
    samples = []
    for _ in range(n_samples):
        ts = int(time.time() * 1000)
        x = round(random.uniform(-1, 1), 2)
        y = round(random.uniform(-1, 1), 2)
        z = round(random.uniform(-1, 1), 2)
        temp = round(random.uniform(20.0, 40.0), 2)
        samples.extend([ts, x, y, z, temp])
        time.sleep(delay_ms / 1000.0)
    return ",".join(map(str, samples))
