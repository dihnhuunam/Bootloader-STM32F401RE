#!/usr/bin/env python3
import argparse
import json
import re
import struct
from pathlib import Path

APP_HEADER_SIZE = 16
APP_HEADER_VERSION_OFFSET = 4


def parse_version(value):
    if isinstance(value, int):
        if value < 0 or value > 0xFFFFFFFF:
            raise ValueError("version integer must fit in uint32_t")
        return value

    if not isinstance(value, str):
        raise ValueError("version must be a string like 1.2.3 or an integer")

    text = value.strip()
    if re.fullmatch(r"0[xX][0-9a-fA-F]+", text):
        parsed = int(text, 16)
        if parsed > 0xFFFFFFFF:
            raise ValueError("version hex value must fit in uint32_t")
        return parsed

    match = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", text)
    if match is None:
        raise ValueError("version string must use MAJOR.MINOR.PATCH format")

    major, minor, patch = (int(part) for part in match.groups())
    if major > 0xFFFF or minor > 0xFF or patch > 0xFF:
        raise ValueError("version limits are major<=65535, minor<=255, patch<=255")

    return (major << 16) | (minor << 8) | patch


def patch_version(path, version):
    raw = bytearray(path.read_bytes())
    if len(raw) < APP_HEADER_SIZE:
        raise ValueError(f"{path} is too small to contain ImageHeader_t")

    struct.pack_into("<I", raw, APP_HEADER_VERSION_OFFSET, version)
    path.write_bytes(raw)


def main():
    parser = argparse.ArgumentParser(description="Patch Application image header version")
    parser.add_argument("--version-json", required=True, help="Path to version.json")
    parser.add_argument("--patched-bin", required=True, help="Full binary containing ImageHeader_t at offset 0")
    parser.add_argument("--header-bin", required=True, help="Patched .app_header section binary")
    args = parser.parse_args()

    version_config = json.loads(Path(args.version_json).read_text(encoding="utf-8"))
    version = parse_version(version_config["version"])

    patch_version(Path(args.patched_bin), version)
    patch_version(Path(args.header_bin), version)

    print(f"Version : 0x{version:08X} ({version_config['version']})")


if __name__ == "__main__":
    main()
