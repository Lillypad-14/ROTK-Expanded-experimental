"""Checks that skill descriptions fit the skill tree text box.

Wraps each "(Level @1)" description string of the given C files the way font_drawLine does, using the
ROM font's glyph widths, and reports descriptions that need more than 3 lines.

    python hacks/tools/skill_text_fit.py hacks/patches/skill_expansion/skill_expansion_content.c

Placeholders are measured at their widest: @1 as "Maxed" or a 2-digit level, @2 as 3 digits.
"""

import argparse
import os
import re
import struct
import sys

ROM = os.path.join(os.path.dirname(__file__), "..", "..", "rotk.gba")
FONT_STYLE_INFOS = 0x08058264
FONT = 0
BOX_WIDTH = 0xD0
MAX_LINES = 3
LINE_BYTES = 63
PLACEHOLDERS = {"1": "Maxed", "2": "000", "3": "00", "4": "00"}


def font_widths(rom, index):
    base = FONT_STYLE_INFOS - 0x08000000 + index * 8
    asset = struct.unpack_from("<I", rom, base)[0] - 0x08000000
    first, last = rom[asset], rom[asset + 1]
    widths_offset = struct.unpack_from(">I", rom, asset + 8)[0]
    widths = rom[asset + widths_offset : asset + widths_offset + last - first + 1]
    return {chr(first + i): w for i, w in enumerate(widths)}, widths[1] if len(widths) > 1 else 4


def width_of(text, widths, fallback):
    return sum(widths.get(c, fallback) for c in text)


def wrap(text, widths, fallback):
    """Lines as font_drawLine selects them (break on the last space that fits)."""
    lines = []
    while text:
        width = 0
        line_end = None
        i = 0
        while i < len(text) and width <= BOX_WIDTH:
            c = text[i]
            if c == "@" and i + 1 < len(text):
                width += width_of(PLACEHOLDERS.get(text[i + 1], "00"), widths, fallback)
                i += 2
                continue
            if c == " ":
                line_end = i
            width += widths.get(c, fallback)
            i += 1
        if i >= len(text) and width <= BOX_WIDTH:
            line_end = len(text)
        if line_end is None or line_end == 0:
            lines.append(text)
            break
        lines.append(text[:line_end])
        text = text[line_end:]
    return lines


def descriptions(path):
    """Concatenated string literals that read like a skill description."""
    src = open(path, encoding="utf-8").read()
    for m in re.finditer(r"\"(?:[^\"\\]|\\.)*\"(?:\s*\"(?:[^\"\\]|\\.)*\")*", src):
        text = "".join(re.findall(r"\"((?:[^\"\\]|\\.)*)\"", m.group(0)))
        if "(Level @1)" in text:
            yield src.count("\n", 0, m.start()) + 1, text


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("files", nargs="+")
    parser.add_argument("--all", action="store_true", help="print every description's wrap")
    args = parser.parse_args()
    rom = open(ROM, "rb").read()
    widths, fallback = font_widths(rom, FONT)
    bad = 0
    for path in args.files:
        for line, text in descriptions(path):
            lines = wrap(text, widths, fallback)
            long_line = any(len(l) > LINE_BYTES for l in lines)
            if len(lines) > MAX_LINES or long_line or args.all:
                status = "OK " if len(lines) <= MAX_LINES and not long_line else "BAD"
                bad += status == "BAD"
                print(f"{status} {path}:{line} ({len(lines)} lines)")
                for l in lines:
                    print(f"      |{l}|")
    print(f"{bad} descriptions overflow")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
