#!/usr/bin/env python3
import socket, csv
from pathlib import Path
from datetime import datetime
from helpers import chunker  # you still have this helper

HOST = socket.gethostbyname(socket.gethostname())
PORT = 3333

START_MARKER = "SENDING_DATA"
END_MARKER   = "DATA_SENT"
COLUMNS      = ["timestamp", "x", "y", "z", "temp"]   # 5 values per sample
UPLOAD_DIR   = Path("uploads")
UPLOAD_DIR.mkdir(exist_ok=True)

def handle_client(conn, addr):
    print(f"[+] {addr} connected")
    buffer, msg, receiving = "", "", False

    while True:
        data = conn.recv(4096)
        if not data:            # client closed socket early
            print(f"[-] {addr} disconnected (early)")
            return

        buffer += data.decode("utf-8")

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

    # ---- save to CSV --------------------------------------------------------
    values = [v for v in msg.split(",") if v.strip()]
    rows = list(chunker(values, 5))   # [ts, x, y, z, temp]

    ts_str = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_path = UPLOAD_DIR / f"session_{ts_str}.csv"   # new file per upload
    # If you’d rather APPEND to one file per day, use:
    # csv_path = UPLOAD_DIR / f"session_{datetime.now():%Y%m%d}.csv"

    is_new = not csv_path.exists()
    with open(csv_path, "a", newline="") as f:
        w = csv.writer(f)
        if is_new:
            w.writerow(COLUMNS)
        for row in rows:
            try:
                ts = int(row[0])                 # keep timestamp integer
                nums = [float(x) for x in row[1:]]
                w.writerow([ts] + nums)
            except ValueError:
                print(f"[!] malformed row skipped: {row}")

    print(f"[√] Stored {len(rows)} samples in {csv_path}")
    conn.sendall(b"OK\n")          # optional ACK
    conn.close()
    print(f"[↘] {addr} done")

def main():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.bind((HOST, PORT))
    srv.listen()
    print(f"[🟢] Buffer-first server listening on {HOST}:{PORT}")

    try:
        while True:                # run forever
            conn, addr = srv.accept()
            handle_client(conn, addr)
    except KeyboardInterrupt:
        print("\n[✋] Shutdown requested by user")
    finally:
        srv.close()

if __name__ == "__main__":
    main()
