"""Unicode mappings for the game's single-byte font encoding."""

# Printable ASCII occupies 0x20..0x7a. Later punctuation slots are repurposed.
CODEMAP = {
    0x7B: "©",
    0x7D: "¡",
    0x7E: "¿",
    0x80: "Á",
    0x83: "Ä",
    0x87: "È",
    0x88: "É",
    0x91: "Ó",
    0x97: "Ú",
    0x98: "Ü",
    0x99: "ß",
    0x9A: "à",
    0x9B: "á",
    0x9C: "â",
    0x9E: "ä",
    0xA1: "ç",
    0xA2: "è",
    0xA3: "é",
    0xA4: "ê",
    0xA5: "ë",
    0xA6: "ì",
    0xA7: "í",
    0xA8: "î",
    0xA9: "ï",
    0xAA: "ñ",
    0xAB: "ò",
    0xAC: "ó",
    0xAD: "ô",
    0xAF: "ö",
    0xB1: "ù",
    0xB2: "ú",
    0xB3: "û",
    0xB4: "ü",
}

CHARMAP = {
    **{chr(byte): byte for byte in range(0x20, 0x7B)},
    **{character: byte for byte, character in CODEMAP.items()},
    "\n": 0x0A,
}


def unicode_to_game(text: str) -> bytes:
    """Encode supported glyphs and newlines without lossy substitutions."""
    result = bytearray()
    for character in text:
        if character not in CHARMAP:
            raise ValueError(
                f"unsupported character {character!r} (U+{ord(character):04X})"
            )
        result.append(CHARMAP[character])
    return bytes(result)


def game_to_unicode(raw: bytes) -> str:
    """Decode font glyphs and printable ASCII, escaping other bytes as \\xNN."""
    chars = []
    for byte in raw:
        if byte in CODEMAP:
            chars.append(CODEMAP[byte])
        elif byte == 0x0A:
            chars.append("\n")
        elif 0x20 <= byte < 0x7F:
            chars.append(chr(byte))
        else:
            chars.append(f"\\x{byte:02x}")
    return "".join(chars)
