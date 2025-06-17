import socket
import csv
import time
from pathlib import Path

CSV_PATH = Path("values.csv")

# chunker
def chunker(seq, size):
    return (seq[pos:pos + size] for pos in range(0, len(seq), size))

# save data to CSV
def save_to_csv(rows, path=CSV_PATH, columns=None):
    print(f"[💾] Saving {len(rows)} rows to {path}")
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
            values = [float(x) for x in first_row]
            avg = sum(values) / len(values)
            print(f"First row: {values} → avg: {avg:.2f}")
        except Exception as e:
            print(f"Failed to parse row: {e}")
    else:
        print("CSV is empty")

    return "07:00"  # hardcoded wake-up time
