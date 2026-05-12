#!/usr/bin/env python3
import argparse
import struct
import sys
import time
from pathlib import Path

START_BYTE = 0x7E
ACK = 0x79
NACK = 0x1F
BLOCK_SIZE = 256
OTA_PACKET_MAGIC = 0x3341544F
OTA_PACKET_HEADER_FORMAT = "<IIIIII"
OTA_PACKET_HEADER_SIZE = 24


def load_serial():
    try:
        import serial
    except ImportError as exc:
        raise SystemExit("pyserial is required. Install with: python -m pip install pyserial") from exc
    return serial


def wait_response(ser, stage):
    response = ser.read(1)
    if response == bytes([ACK]):
        return
    if response == bytes([NACK]):
        raise SystemExit(f"{stage}: received NACK")
    if response == b"":
        raise SystemExit(f"{stage}: response timeout")
    raise SystemExit(f"{stage}: unexpected response 0x{response[0]:02X}")


def read_target_slot(ser):
    response = ser.read(1)
    if response == b"":
        raise SystemExit("target slot: response timeout")

    target_slot = response[0]
    if target_slot not in (0, 1):
        raise SystemExit(f"target slot: unexpected value 0x{target_slot:02X}")

    return target_slot


def send_image_blocks(ser, data, offset, size, block_size, sent, total, label, last_percent):
    remaining = size

    while remaining > 0:
        chunk_size = min(block_size, remaining)
        chunk = data[offset : offset + chunk_size]
        ser.write(chunk)
        ser.flush()
        wait_response(ser, f"{label} block")

        offset += chunk_size
        sent += chunk_size
        remaining -= chunk_size

        percent = int((sent * 100) / total)
        if percent != last_percent:
            print(f"\rSent   : {sent}/{total} bytes ({percent}%)", end="")
            last_percent = percent

    return offset, sent, last_percent


def main():
    parser = argparse.ArgumentParser(description="Send combined OTA packet to MCU over UART")
    parser.add_argument("--port", required=True, help="Serial port, for example COM5")
    parser.add_argument("--baudrate", type=int, default=115200, help="UART baudrate")
    parser.add_argument("--packet", required=True, help="Path to Application_ota_packet.bin")
    parser.add_argument("--block-size", type=int, default=BLOCK_SIZE, help="Write block size")
    parser.add_argument("--startup-delay", type=float, default=0.2, help="Delay after opening port in seconds")
    parser.add_argument("--response-timeout", type=float, default=15.0, help="ACK/NACK response timeout in seconds")
    args = parser.parse_args()

    packet_path = Path(args.packet)
    data = packet_path.read_bytes()
    if len(data) < OTA_PACKET_HEADER_SIZE:
        raise SystemExit("OTA packet is too small")

    magic, version, slot_a_size, slot_a_crc32, slot_b_size, slot_b_crc32 = struct.unpack_from(
        OTA_PACKET_HEADER_FORMAT, data, 0
    )
    if magic != OTA_PACKET_MAGIC:
        raise SystemExit(f"Invalid OTA packet magic: 0x{magic:08X}")

    expected_size = OTA_PACKET_HEADER_SIZE + slot_a_size + slot_b_size
    if len(data) != expected_size:
        raise SystemExit(f"OTA packet size mismatch: expected {expected_size}, got {len(data)}")

    serial = load_serial()

    if args.block_size <= 0 or args.block_size > BLOCK_SIZE:
        raise SystemExit(f"--block-size must be between 1 and {BLOCK_SIZE}")

    print(f"Port   : {args.port}")
    print(f"Baud   : {args.baudrate}")
    print(f"Packet : {packet_path}")
    print(f"Size   : {len(data)} bytes")
    print(f"Slot A : {slot_a_size} bytes CRC32=0x{slot_a_crc32:08X}")
    print(f"Slot B : {slot_b_size} bytes CRC32=0x{slot_b_crc32:08X}")

    with serial.Serial(args.port, args.baudrate, timeout=args.response_timeout, write_timeout=5) as ser:
        time.sleep(args.startup_delay)
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        ser.write(bytes([START_BYTE]))
        ser.flush()
        wait_response(ser, "start")
        target_slot = read_target_slot(ser)

        if target_slot == 0:
            image_offset = OTA_PACKET_HEADER_SIZE
            image_size = slot_a_size
            image_crc32 = slot_a_crc32
            image_label = "slot A"
        else:
            image_offset = OTA_PACKET_HEADER_SIZE + slot_a_size
            image_size = slot_b_size
            image_crc32 = slot_b_crc32
            image_label = "slot B"

        print(f"Target : {image_label} ({image_size} bytes CRC32=0x{image_crc32:08X})")

        ser.write(struct.pack("<I", image_size))
        ser.flush()
        wait_response(ser, "image size")
        wait_response(ser, "image ready")

        sent = 0
        total = image_size
        last_percent = -1
        offset = image_offset

        offset, sent, last_percent = send_image_blocks(
            ser, data, offset, image_size, args.block_size, sent, total, image_label, last_percent
        )

        wait_response(ser, "finish")

    print("\nDone")
    return 0


if __name__ == "__main__":
    sys.exit(main())
