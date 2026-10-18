"""Shared font decoding and encoding."""

from rotkit.text_encoding import CHARMAP, game_to_unicode, unicode_to_game


def test_game_to_unicode_empty() -> None:
    assert game_to_unicode(b"") == ""


def test_game_to_unicode_printable_ascii() -> None:
    assert game_to_unicode(bytes(range(0x20, 0x7B))) == "".join(
        chr(byte) for byte in range(0x20, 0x7B)
    )


def test_game_to_unicode_repurposed_punctuation() -> None:
    assert game_to_unicode(b"\x7b\x7c\x7d\x7e") == "©|¡¿"


def test_game_to_unicode_accents_newlines_and_placeholders() -> None:
    assert game_to_unicode(b"\x87\xa2 Pr\x9ezision\n@1") == "Èè Präzision\n@1"


def test_game_to_unicode_unmapped_bytes() -> None:
    assert game_to_unicode(b"\x00\x09\x0d\x1f\x7f\x81\xff") == (
        r"\x00\x09\x0d\x1f\x7f\x81\xff"
    )


def test_supported_glyphs_round_trip() -> None:
    text = "".join(CHARMAP)
    raw = bytes(CHARMAP.values())
    assert game_to_unicode(unicode_to_game(text)) == text
    assert unicode_to_game(game_to_unicode(raw)) == raw
