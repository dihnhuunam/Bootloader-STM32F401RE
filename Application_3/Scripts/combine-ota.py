#!/usr/bin/env python3
import argparse
import struct
import zlib
from pathlib import Path

OTA_PACKET_MAGIC = 0x3341544F
OTA_PACKET_HEADER_FORMAT = "<IIIIII"


def read_image(path):
    data = Path(path).read_bytes()
    return data, zlib.crc32(data) & 0xFFFFFFFF


def main():
    parser = argparse.ArgumentParser(description="Combine Slot A/B firmware images into one OTA packet")
    parser.add_argument("--slot-a-bin", required=True, help="CRC-patched binary linked for Slot A")
    parser.add_argument("--slot-b-bin", required=True, help="CRC-patched binary linked for Slot B")
    parser.add_argument("--version-json", required=True, help="Path to version.json")
    parser.add_argument("--output", required=True, help="Output combined OTA packet")
    args = parser.parse_args()

    slot_a, slot_a_crc32 = read_image(args.slot_a_bin)
    slot_b, slot_b_crc32 = read_image(args.slot_b_bin)

    import json

    version_config = json.loads(Path(args.version_json).read_text(encoding="utf-8"))
    version_text = str(version_config["version"]).strip()
    version_parts = version_text.split(".")
    if len(version_parts) != 3:
        raise ValueError("version string must use MAJOR.MINOR.PATCH format")

    major, minor, patch = (int(part) for part in version_parts)
    version = (major << 16) | (minor << 8) | patch

    header = struct.pack(
        OTA_PACKET_HEADER_FORMAT,
        OTA_PACKET_MAGIC,
        version,
        len(slot_a),
        slot_a_crc32,
        len(slot_b),
        slot_b_crc32,
    )

    output_path = Path(args.output)
    output_path.write_bytes(header + slot_a + slot_b)

    print(f"OTA packet : {output_path}")
    print(f"Version    : 0x{version:08X} ({version_text})")
    print(f"Slot A     : {len(slot_a)} bytes CRC32=0x{slot_a_crc32:08X}")
    print(f"Slot B     : {len(slot_b)} bytes CRC32=0x{slot_b_crc32:08X}")


if __name__ == "__main__":
    main()
