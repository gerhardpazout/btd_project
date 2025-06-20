#!/usr/bin/env python3
import socket, csv, errno
from pathlib import Path
from datetime import datetime
from helpers import chunker

HOST, PORT = socket.gethostbyname(socket.gethostname()), 3333
START_MARKER, END_MARKER = "SENDING_DATA", "DATA_SENT"
COLUMNS = ["timestamp", "x", "y", "z", "temp"]

#####################################################################
# create one folder per server run
UPLOAD_ROOT   = Path("uploads")
SESSION_DIR   = UPLOAD_ROOT / f"session_{datetime.now():%Y%m%d_%H%M}"
SESSION_DIR.mkdir(parents=True, exist_ok=True)
print(f"Info: Session directory: {SESSION_DIR}")
#####################################################################

def handle_client(conn, addr):
    print(f"Info: {addr} connected")
    buffer, msg, receiving = "", "", False

    while True:
        chunk = conn.recv(4096)
        if not chunk:
            print(f"Info: {addr} disconnected early")
            return
        buffer += chunk.decode()

        if not receiving:
            if START_MARKER in buffer:
                receiving = True
                buffer = buffer.split(START_MARKER, 1)[1]
            else:
                continue

        if END_MARKER in buffer:
            msg += buffer.split(END_MARKER, 1)[0]
            break
        else:
            msg += buffer
            buffer = ""

    # -------- save one CSV per upload -----------------------------
    rows = []
    for line in msg.strip().splitlines():
        parts = [p.strip() for p in line.split(",")]
        if len(parts) == 5:
            rows.append(parts)
        else:
            print(f"Warning: Skipping malformed line: {line}")

    ts_file = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_path = SESSION_DIR / f"batch_{ts_file}.csv"

    with open(csv_path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(COLUMNS)
        for row in rows:
            try:
                ts_int  = int(row[0])
                floats  = [float(v) for v in row[1:]]
                w.writerow([ts_int] + floats)
            except ValueError:
                print(f"Error: malformed row skipped: {row}")

    print(f"Info: {len(rows)} samples → {csv_path.name}")
    conn.sendall(b"OK\n")
    conn.close()
    print(f"{addr} done")

def main():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.bind((HOST, PORT))
    srv.listen()
    srv.settimeout(2.0)  # ✨ important: make accept() non-blocking
    print(f"Listening on {HOST}:{PORT}")

    try:
        while True:
            try:
                conn, addr = srv.accept()
                handle_client(conn, addr)
            except socket.timeout:
                continue  # no client → check for Ctrl+C again
    except KeyboardInterrupt:
        print("\nServer stopped by user")
    finally:
        srv.close()

if __name__ == "__main__":
    main()
