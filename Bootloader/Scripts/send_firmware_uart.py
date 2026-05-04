import argparse
import time

import serial

START_BYTE = 0xAA
ACK = 0x79
NACK = 0x1F


def parse_args():
    parser = argparse.ArgumentParser(description="Send firmware .bin to STM32 bootloader over UART.")
    parser.add_argument("--port", required=True, help="Serial port, for example COM5.")
    parser.add_argument("--baud", type=int, default=115200, help="UART baudrate.")
    parser.add_argument("--bin", required=True, help="CRC-patched firmware .bin built for Slot B.")
    parser.add_argument("--chunk", type=int, default=256, help="Payload chunk size.")
    parser.add_argument("--delay", type=float, default=0.01, help="Delay after each ACK in seconds.")
    parser.add_argument("--retries", type=int, default=3, help="Retry count per packet.")
    return parser.parse_args()


def checksum(data):
    return sum(data) & 0xFFFF


def make_packet(data):
    length = len(data)
    crc = checksum(data)
    return bytes([START_BYTE, (length >> 8) & 0xFF, length & 0xFF]) + data + bytes([(crc >> 8) & 0xFF, crc & 0xFF])


def make_end_packet():
    return bytes([START_BYTE, 0x00, 0x00])


def wait_ack(uart):
    response = uart.read(1)
    return len(response) == 1 and response[0] == ACK


def send_packet(uart, packet, retries):
    for _ in range(retries):
        uart.write(packet)
        uart.flush()
        if wait_ack(uart):
            return True
    return False


def main():
    args = parse_args()

    with open(args.bin, "rb") as firmware_file:
        firmware = firmware_file.read()

    with serial.Serial(args.port, args.baud, timeout=2) as uart:
        time.sleep(0.2)

        sent = 0
        while sent < len(firmware):
            chunk = firmware[sent : sent + args.chunk]
            packet = make_packet(chunk)
            if not send_packet(uart, packet, args.retries):
                raise RuntimeError(f"Packet failed at offset {sent}")
            sent += len(chunk)
            print(f"\rSent {sent}/{len(firmware)} bytes", end="")
            time.sleep(args.delay)

        if not send_packet(uart, make_end_packet(), args.retries):
            raise RuntimeError("End packet failed")

    print("\nDone")


if __name__ == "__main__":
    main()
