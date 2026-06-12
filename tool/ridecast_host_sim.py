#!/usr/bin/env python3
"""
ridecast_host_sim.py

Simulates the Host PC that:
  1. Connects to RideCast over UART (serial port)
  2. Sends WiFi START command (SSID + password)
  3. Receives STATUS frames and DATA frames
  4. Prints everything decoded
  
  
# Use all defaults from script
python ridecast_host_sim.py

# Override just port
python ridecast_host_sim.py --port COM5

# Override everything
python ridecast_host_sim.py --port COM3 --baud 115200 --ssid MyAP --pass mypass --timeout 3.0

[CFG] port=COM3  baud=115200  ssid=MyWiFi  timeout=2.0s
[INFO] Opened COM3 @ 115200 baud

[INFO] Sending WiFi START: ssid=MyWiFi
[TX] CMD START  (frame: 0180606d7957694669...)

[INFO] Waiting for device responses (Ctrl+C to stop)...

[RX] [WiFi ]  STATUS    CONNECTED
[RX] [WiFi ]  STATUS    RETRYING
[RX] [WiFi ]  STATUS    CONNECTED
[RX] [WiFi ]  STATUS    READY
[RX] [WiFi ]  DATA      NAV       len= 120  NAV,42,270,12.971600,77.594600,LEFT,320,MG Road
[RX] [WiFi ]  DATA      NAV       len= 125  NAV,45,275,12.972000,77.596000,LEFT,320,MG Road
[RX] [WiFi ]  DATA      NAV       len= 122  NAV,40,268,12.972500,77.597500,STRAIGHT,800,Brigade Road


"""

import argparse
import serial
import sys
import struct
import threading
import time

# ---------------------------------------------------------------------------
# DEFAULTS - change these if not passing args
# ---------------------------------------------------------------------------
DEFAULT_PORT = "COM3"
DEFAULT_BAUD = 115200
DEFAULT_SSID = "RideCast-AP"
DEFAULT_PASS = "password123"
DEFAULT_TIMEOUT = 2.0

# ---------------------------------------------------------------------------
# rc_protocol constants (match C header)
# ---------------------------------------------------------------------------
RC_HDR_INTF_MASK  = 0x03
RC_HDR_INTF_SHIFT = 0
RC_HDR_TYPE_MASK  = 0x0C
RC_HDR_TYPE_SHIFT = 2
RC_HDR_SUB_MASK   = 0xF0
RC_HDR_SUB_SHIFT  = 4

RC_MAX_PAYLOAD_LEN = 4096
RC_FRAME_HEADER_LEN = 1
RC_FRAME_LEN_FIELD = 2
RC_MIN_DATA_FRAME_LEN = RC_FRAME_HEADER_LEN + RC_FRAME_LEN_FIELD

RC_WIFI_SSID_LEN = 32
RC_WIFI_PASS_LEN = 64
RC_WIFI_START_PARAMS = RC_WIFI_SSID_LEN + RC_WIFI_PASS_LEN

# Interface
INTF_WIFI = 0x01
INTF_BLE = 0x02
INTF_BT = 0x03

# Type
TYPE_CMD = 0x00
TYPE_STATUS = 0x01
TYPE_DATA = 0x02
TYPE_ACK = 0x03

# CMD subcodes
CMD_START = 0x00
CMD_STOP = 0x01
CMD_RECONNECT = 0x02

# STATUS subcodes
STS_OK = 0x00
STS_FAIL = 0x01
STS_CONNECTED = 0x02
STS_DISCONNECTED = 0x03
STS_AUTH_OK = 0x04
STS_AUTH_FAIL = 0x05
STS_FAILED = 0x06
STS_SLEEPING = 0x07
STS_READY = 0x08
STS_RETRYING = 0x09

# DATA subcodes
DATA_RAW = 0x00
DATA_NAV = 0x01
DATA_NOTIF = 0x02

INTF_NAME = {
    INTF_WIFI: "WiFi",
    INTF_BLE: "BLE",
    INTF_BT: "BT",
}

STS_NAME = {
    STS_OK: "OK",
    STS_FAIL: "FAIL",
    STS_CONNECTED: "CONNECTED",
    STS_DISCONNECTED: "DISCONNECTED",
    STS_AUTH_OK: "AUTH_OK",
    STS_AUTH_FAIL: "AUTH_FAIL",
    STS_FAILED: "FAILED",
    STS_SLEEPING: "SLEEPING",
    STS_READY: "READY",
    STS_RETRYING: "RETRYING",
}

DATA_NAME = {
    DATA_RAW: "RAW",
    DATA_NAV: "NAV",
    DATA_NOTIF: "NOTIF",
}


def make_header(intf, typ, subcode):
    return ((intf & 0x03) | ((typ & 0x03) << 2) | ((subcode & 0x0F) << 4))


def encode_cmd_start_wifi(ssid: str, passwd: str) -> bytes:
    """Encode WiFi START command: header + length + SSID(32) + PASS(64)"""
    buf = bytearray(3 + RC_WIFI_START_PARAMS)

    buf[0] = make_header(INTF_WIFI, TYPE_CMD, CMD_START)
    buf[1] = (RC_WIFI_START_PARAMS >> 8) & 0xFF
    buf[2] = RC_WIFI_START_PARAMS & 0xFF

    # Fill SSID
    ssid_bytes = ssid.encode("utf-8")
    for i in range(min(len(ssid_bytes), RC_WIFI_SSID_LEN - 1)):
        buf[3 + i] = ssid_bytes[i]

    # Fill PASS
    pass_bytes = passwd.encode("utf-8")
    for i in range(min(len(pass_bytes), RC_WIFI_PASS_LEN - 1)):
        buf[3 + RC_WIFI_SSID_LEN + i] = pass_bytes[i]

    return bytes(buf)


def decode_frame(raw_buf: bytes):
    """Decode a single frame from raw buffer. Returns (frame_dict, bytes_consumed) or (None, 0)"""
    if len(raw_buf) < RC_FRAME_HEADER_LEN:
        return None, 0

    hdr = raw_buf[0]
    intf = (hdr & RC_HDR_INTF_MASK) >> RC_HDR_INTF_SHIFT
    typ = (hdr & RC_HDR_TYPE_MASK) >> RC_HDR_TYPE_SHIFT
    subcode = (hdr & RC_HDR_SUB_MASK) >> RC_HDR_SUB_SHIFT

    frame = {"intf": intf, "type": typ, "subcode": subcode, "payload": None}

    if typ == TYPE_DATA:
        # DATA frame: header + 2-byte length + payload
        if len(raw_buf) < RC_MIN_DATA_FRAME_LEN:
            return None, 0

        pay_len = (raw_buf[1] << 8) | raw_buf[2]

        if pay_len > RC_MAX_PAYLOAD_LEN or len(raw_buf) < 3 + pay_len:
            return None, 0

        frame["payload"] = raw_buf[3 : 3 + pay_len]
        return frame, 3 + pay_len
    else:
        # CMD / STATUS / ACK: header only
        return frame, 1


def format_frame(frame):
    """Pretty-print a decoded frame"""
    intf_str = INTF_NAME.get(frame["intf"], f"UNK({frame['intf']})")
    type_str = ["CMD", "STATUS", "DATA", "ACK"][frame["type"]]

    if frame["type"] == TYPE_STATUS:
        sub_str = STS_NAME.get(frame["subcode"], f"UNK({frame['subcode']})")
        return f"[{intf_str:5s}] {type_str:8s}  {sub_str}"
    elif frame["type"] == TYPE_DATA:
        sub_str = DATA_NAME.get(frame["subcode"], f"UNK({frame['subcode']})")
        pay_preview = (
            frame["payload"].decode("utf-8", errors="replace")[:60]
            if frame["payload"]
            else ""
        )
        return f"[{intf_str:5s}] {type_str:8s}  {sub_str:8s}  len={len(frame['payload']):4d}  {pay_preview}"
    else:
        return f"[{intf_str:5s}] {type_str:8s}"


def rx_thread_func(ser: serial.Serial, running: list):
    """Background thread: read frames from serial and print"""
    buf = bytearray()

    while running[0]:
        try:
            chunk = ser.read(1024)
            if not chunk:
                time.sleep(0.01)
                continue

            buf.extend(chunk)

            # Try to decode frames from buffer
            while len(buf) > 0:
                frame, consumed = decode_frame(bytes(buf))
                if frame is None:
                    break

                print(f"[RX] {format_frame(frame)}")

                # Remove consumed bytes
                buf = buf[consumed:]

        except Exception as e:
            print(f"[ERR] RX thread: {e}")
            break


def main():
    p = argparse.ArgumentParser(description="RideCast Host simulator")
    p.add_argument("--port", default=None, help=f"Serial port (default: {DEFAULT_PORT})")
    p.add_argument("--baud", type=int, default=None, help=f"Baud rate (default: {DEFAULT_BAUD})")
    p.add_argument("--ssid", default=None, help=f"WiFi SSID (default: {DEFAULT_SSID})")
    p.add_argument("--pass", default=None, help=f"WiFi password (default: {DEFAULT_PASS})")
    p.add_argument("--timeout", type=float, default=None, help=f"Serial timeout (default: {DEFAULT_TIMEOUT}s)")
    args = p.parse_args()

    # Use arg if given, else fall back to defaults
    port = args.port or DEFAULT_PORT
    baud = args.baud or DEFAULT_BAUD
    ssid = args.ssid or DEFAULT_SSID
    passwd = args.pass or DEFAULT_PASS
    timeout = args.timeout or DEFAULT_TIMEOUT

    print(f"[CFG] port={port}  baud={baud}  ssid={ssid}  timeout={timeout}s")

    try:
        ser = serial.Serial(port, baud, timeout=timeout)
        print(f"[INFO] Opened {port} @ {baud} baud")
        time.sleep(1.0)  # Let device enumerate

        # Start RX thread
        running = [True]
        t = threading.Thread(target=rx_thread_func, args=(ser, running), daemon=True)
        t.start()

        # Send WiFi START command
        print(f"\n[INFO] Sending WiFi START: ssid={ssid}")
        cmd_frame = encode_cmd_start_wifi(ssid, passwd)
        ser.write(cmd_frame)
        print(f"[TX] CMD START  (frame: {cmd_frame.hex()})")

        # Wait and collect responses
        print("\n[INFO] Waiting for device responses (Ctrl+C to stop)...\n")

        while True:
            time.sleep(0.5)

    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")
    except Exception as e:
        print(f"[ERR] {e}")
        sys.exit(1)
    finally:
        running[0] = False
        try:
            ser.close()
        except Exception:
            pass
        print("[INFO] Done.")


if __name__ == "__main__":
    main()