#!/usr/bin/env python3
"""
ridecast_maps_sim.py

# use defaults hardcoded in script - no args needed
python ridecast_maps_sim.py

# override just host
python ridecast_maps_sim.py --host 192.168.1.105

# override everything
python ridecast_maps_sim.py --host 192.168.1.105 --port 5555 --token rc_secret_v1 --interval 0.5
"""

import argparse
import socket
import sys
import threading
import time
import math
import random

# ---------------------------------------------------------------------------
# CONFIG - change these if not passing as args
# ---------------------------------------------------------------------------
DEFAULT_HOST = "192.168.4.1"
DEFAULT_PORT = 5555
DEFAULT_TOKEN = "rc_secret_v1"
DEFAULT_INTERVAL = 1.0

# ---------------------------------------------------------------------------
# fmt: off
ROUTE = [
    {"lat": 12.9716, "lon": 77.5946, "street": "MG Road",       "turn": "STRAIGHT", "dist": 500},
    {"lat": 12.9720, "lon": 77.5960, "street": "MG Road",       "turn": "LEFT",     "dist": 320},
    {"lat": 12.9725, "lon": 77.5975, "street": "Brigade Road",  "turn": "STRAIGHT", "dist": 800},
    {"lat": 12.9730, "lon": 77.5990, "street": "Brigade Road",  "turn": "RIGHT",    "dist": 150},
    {"lat": 12.9740, "lon": 77.6005, "street": "Church Street", "turn": "STRAIGHT", "dist": 600},
    {"lat": 12.9750, "lon": 77.6020, "street": "Church Street", "turn": "LEFT",     "dist": 200},
    {"lat": 12.9760, "lon": 77.6035, "street": "Lavelle Road",  "turn": "ARRIVE",   "dist": 0  },
]
# fmt: on


def heading_deg(a, b):
    d_lat = b["lat"] - a["lat"]
    d_lon = b["lon"] - a["lon"]
    return int((math.degrees(math.atan2(d_lon, d_lat)) + 360) % 360)


def rx_loop(sock):
    try:
        while True:
            data = sock.recv(1024)
            if not data:
                print("\n[INFO] Device closed connection.")
                break
            print(f"\n[RX] {data.decode('utf-8', errors='replace').rstrip()}")
    except Exception:
        pass


def stream_nav(sock, interval):
    print("\n[INFO] Streaming NAV data  (Ctrl+C to stop)\n")
    print(f"{'#':<3} {'STREET':<20} {'TURN':<10} {'SPD':>4} {'HDG':>4}  FRAME")
    print("-" * 75)

    speed = 30
    for idx, wp in enumerate(ROUTE):
        next_wp = ROUTE[min(idx + 1, len(ROUTE) - 1)]
        heading = heading_deg(wp, next_wp)
        speed = max(10, min(80, speed + random.randint(-5, 5)))

        line = (
            f"NAV,{speed},{heading},"
            f"{wp['lat']:.6f},{wp['lon']:.6f},"
            f"{wp['turn']},{wp['dist']},{wp['street']}\n"
        )

        try:
            sock.sendall(line.encode("utf-8"))
        except Exception as e:
            print(f"[ERR] Send failed: {e}")
            break

        print(
            f"{idx:<3} {wp['street']:<20} {wp['turn']:<10} {speed:>4} {heading:>4}  {line.rstrip()}"
        )

        if wp["turn"] == "ARRIVE":
            print("\n[INFO] Arrived at destination.")
            break

        time.sleep(interval)


def main():
    p = argparse.ArgumentParser(description="RideCast Maps simulator")
    p.add_argument("--host", default=None, help=f"Device IP  (default: {DEFAULT_HOST})")
    p.add_argument(
        "--port", type=int, default=None, help=f"TCP port   (default: {DEFAULT_PORT})"
    )
    p.add_argument(
        "--token", default=None, help=f"Auth token (default: {DEFAULT_TOKEN})"
    )
    p.add_argument(
        "--interval",
        type=float,
        default=None,
        help=f"Seconds between NAV frames (default: {DEFAULT_INTERVAL})",
    )
    args = p.parse_args()

    # Use arg if given, else fall back to script-level defaults
    host = args.host or DEFAULT_HOST
    port = args.port or DEFAULT_PORT
    token = args.token or DEFAULT_TOKEN
    interval = args.interval or DEFAULT_INTERVAL

    print(f"[CFG] host={host}  port={port}  token={token}  interval={interval}s")

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(8.0)

    try:
        print(f"[INFO] Connecting to {host}:{port} ...")
        sock.connect((host, port))
        sock.settimeout(None)
        print("[INFO] Connected.")

        sock.sendall(f"TOKEN {token}\n".encode("utf-8"))
        print(f"[INFO] Auth sent.")
        time.sleep(0.5)

        threading.Thread(target=rx_loop, args=(sock,), daemon=True).start()

        stream_nav(sock, interval)

    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")
    except Exception as e:
        print(f"[ERR] {e}")
        sys.exit(1)
    finally:
        try:
            sock.close()
        except Exception:
            pass
        print("[INFO] Done.")


if __name__ == "__main__":
    main()
