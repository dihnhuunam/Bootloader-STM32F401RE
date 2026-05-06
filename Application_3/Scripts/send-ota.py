#!/usr/bin/env python3
import argparse
import sys
import time
from pathlib import Path


def load_serial():
    try:
        import serial
    except ImportError as exc:
        raise SystemExit("pyserial is required. Install with: python -m pip install pyserial") from exc
    return serial


def main():
    parser = argparse.ArgumentParser(description="Send combined OTA packet to MCU over UART")
    parser.add_argument("--port", required=True, help="Serial port, for example COM5")
    parser.add_argument("--baudrate", type=int, default=115200, help="UART baudrate")
    parser.add_argument("--packet", required=True, help="Path to Application_3_ota_packet.bin")
    parser.add_argument("--chunk-size", type=int, default=256, help="Write chunk size")
    parser.add_argument("--inter-chunk-delay", type=float, default=0.002, help="Delay between chunks in seconds")
    parser.add_argument("--startup-delay", type=float, default=0.2, help="Delay after opening port in seconds")
    args = parser.parse_args()

    packet_path = Path(args.packet)
    data = packet_path.read_bytes()
    serial = load_serial()

    if args.chunk_size <= 0:
        raise SystemExit("--chunk-size must be positive")

    print(f"Port   : {args.port}")
    print(f"Baud   : {args.baudrate}")
    print(f"Packet : {packet_path}")
    print(f"Size   : {len(data)} bytes")

    with serial.Serial(args.port, args.baudrate, timeout=1, write_timeout=5) as ser:
        time.sleep(args.startup_delay)
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        ser.write(b"U")
        ser.flush()
        time.sleep(args.inter_chunk_delay)

        sent = 0
        total = len(data)
        last_percent = -1

        while sent < total:
            chunk = data[sent : sent + args.chunk_size]
            ser.write(chunk)
            ser.flush()
            sent += len(chunk)

            percent = int((sent * 100) / total)
            if percent != last_percent:
                print(f"\rSent   : {sent}/{total} bytes ({percent}%)", end="")
                last_percent = percent

            if args.inter_chunk_delay > 0:
                time.sleep(args.inter_chunk_delay)

    print("\nDone")
    return 0


if __name__ == "__main__":
    sys.exit(main())
