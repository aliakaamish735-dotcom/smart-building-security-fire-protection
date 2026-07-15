#!/usr/bin/env python3
"""
serial_csv_logger.py

Captures the CSV stream printed by the Arduino sketch over the USB
serial connection and writes it to a timestamped .csv file on your
computer, so it's ready to open in Excel / MATLAB / Python for the
Sensors & Instrumentation report.

The Arduino only ever prints text over serial - it can't write a file
to disk itself. This script is the "Serial-to-CSV" side of that: it
just reads whatever the board prints, line by line, and saves it.

USAGE
-----
1. Install the one dependency (only needs to be done once):
     pip install pyserial

2. Find your board's serial port:
     - Windows: something like COM3, COM4, ...
       (Check Device Manager -> Ports (COM & LPT), or the Arduino IDE's
       Tools -> Port menu while the board is plugged in.)
     - macOS:   something like /dev/cu.usbmodem14101
       (Check the Arduino IDE's Tools -> Port menu, or run
       `ls /dev/cu.*` in Terminal.)
     - Linux:   something like /dev/ttyACM0 or /dev/ttyUSB0
       (Check the Arduino IDE's Tools -> Port menu, or run
       `ls /dev/tty*` before/after plugging in the board.)

3. Run the script, pointing it at that port:
     python serial_csv_logger.py --port COM3
     python serial_csv_logger.py --port /dev/cu.usbmodem14101
     python serial_csv_logger.py --port /dev/ttyACM0

   It will:
     - open the port at 9600 baud (matches Serial.begin(9600) in the sketch)
     - wait for the board to reset (opening the port resets an Uno)
     - capture every line it prints into a new file named
       sensor_log_YYYYMMDD_HHMMSS.csv in the current folder
     - print each row to your terminal too, so you can watch it live

4. Trigger a calibration run (optional) by typing one of these and
   pressing Enter while the script is running:
     CAL:LOADCELL
     CAL:FLOW
     CAL:TEMP
     CAL:HUMIDITY
     CAL:OPTICAL
   Those calibration rows get captured into the same CSV file.

5. Stop logging with Ctrl+C whenever you're done. The file is flushed
   after every line, so it's safe even if you don't stop it cleanly.

OPTIONS
-------
  --port PORT        Serial port (required)
  --baud BAUD         Baud rate (default: 9600, matches the sketch)
  --outfile PATH       Exact output filename instead of an auto-timestamped one
  --seconds N          Stop automatically after N seconds (default: run until Ctrl+C)
"""

import argparse
import csv
import sys
import threading
import time
from datetime import datetime

try:
    import serial
except ImportError:
    sys.exit(
        "The 'pyserial' package is required but not installed.\n"
        "Install it with:\n\n    pip install pyserial\n"
    )


def input_forwarder(ser):
    """
    Runs in a background thread. Reads lines you type in the terminal
    (e.g. CAL:LOADCELL) and sends them to the Arduino over serial, so
    you can trigger a calibration routine while logging is running.
    """
    while True:
        try:
            line = sys.stdin.readline()
        except (EOFError, ValueError):
            return
        if not line:
            return
        line = line.strip()
        if not line:
            continue
        try:
            ser.write((line + "\n").encode("utf-8"))
        except serial.SerialException:
            return


def main():
    parser = argparse.ArgumentParser(description="Capture Arduino CSV serial output to a .csv file.")
    parser.add_argument("--port", required=True, help="Serial port, e.g. COM3 or /dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=9600, help="Baud rate (default: 9600)")
    parser.add_argument("--outfile", default=None, help="Output CSV filename (default: auto-timestamped)")
    parser.add_argument("--seconds", type=float, default=None, help="Stop automatically after N seconds")
    args = parser.parse_args()

    outfile = args.outfile or f"sensor_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"Opening {args.port} at {args.baud} baud...")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        sys.exit(f"Could not open {args.port}: {e}")

    # Opening the port resets an Uno; give it a couple seconds to boot
    # and print its header row before we start reading in earnest.
    time.sleep(2)

    print(f"Logging to {outfile}")
    print("Press Ctrl+C to stop.")
    print("Type a command (e.g. CAL:LOADCELL) and press Enter any time to send it to the board.\n")

    forwarder = threading.Thread(target=input_forwarder, args=(ser,), daemon=True)
    forwarder.start()

    start_time = time.monotonic()
    rows_written = 0

    try:
        with open(outfile, "w", newline="") as f:
            writer = csv.writer(f)
            while True:
                if args.seconds is not None and (time.monotonic() - start_time) >= args.seconds:
                    print(f"\nReached {args.seconds}s limit, stopping.")
                    break

                raw_line = ser.readline()
                if not raw_line:
                    continue  # read timeout, no data this cycle - keep waiting

                line = raw_line.decode("utf-8", errors="replace").strip()
                if not line:
                    continue

                print(line)
                # Split on commas so each field lands in its own CSV
                # column properly, rather than writing the raw string.
                writer.writerow(line.split(","))
                f.flush()
                rows_written += 1

    except KeyboardInterrupt:
        print("\nStopped by user.")
    finally:
        ser.close()
        print(f"\nSaved {rows_written} rows to {outfile}")


if __name__ == "__main__":
    main()
