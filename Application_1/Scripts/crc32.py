#!/usr/bin/env python3
import argparse
import struct
import zlib
from pathlib import Path

APP_HEADER_SIZE = 16
APP_HEADER_CRC32_OFFSET = 8


def main():
    parser = argparse.ArgumentParser(
        description="Calculate CRC32 and patch Application image header"
    )
    parser.add_argument("bin_file", nargs="?", help="Path to .bin file for print-only mode")
    parser.add_argument("--payload-bin", help="Binary used for CRC calculation")
    parser.add_argument("--raw-bin", help="Full raw binary containing .app_header at offset 0")
    parser.add_argument("--patched-bin", help="Output full binary with patched CRC32 header")
    parser.add_argument("--header-bin", help="Output patched .app_header section binary")
    args = parser.parse_args()

    patch_args = [args.payload_bin, args.raw_bin, args.patched_bin, args.header_bin]
    if any(patch_args):
        if not all(patch_args):
            parser.error("--payload-bin, --raw-bin, --patched-bin, and --header-bin must be used together")

        payload_path = Path(args.payload_bin)
        raw_path = Path(args.raw_bin)
        patched_path = Path(args.patched_bin)
        header_path = Path(args.header_bin)

        payload = payload_path.read_bytes()
        raw = bytearray(raw_path.read_bytes())

        if len(raw) < APP_HEADER_SIZE:
            raise ValueError("Raw binary is too small to contain ImageHeader_t")

        crc32 = zlib.crc32(payload) & 0xFFFFFFFF
        struct.pack_into("<I", raw, APP_HEADER_CRC32_OFFSET, crc32)

        patched_path.write_bytes(raw)
        header_path.write_bytes(raw[:APP_HEADER_SIZE])

        print(f"Payload : {payload_path}")
        print(f"Size    : {len(payload)} bytes")
        print(f"CRC32   : 0x{crc32:08X}")
        print(f"BIN     : {patched_path}")
        print(f"Header  : {header_path}")
        return

    if args.bin_file is None:
        parser.error("bin_file is required in print-only mode")

    path = Path(args.bin_file)

    if not path.is_file():
        raise FileNotFoundError(f"File not found: {path}")

    data = path.read_bytes()

    size = len(data)
    crc32 = zlib.crc32(data) & 0xFFFFFFFF

    print(f"File : {path}")
    print(f"Size : {size} bytes")
    print(f"CRC32: 0x{crc32:08X}")


if __name__ == "__main__":
    main()
