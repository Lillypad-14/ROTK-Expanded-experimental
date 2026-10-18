"""UTF-8 override parsing, font encoding, and include generation."""

import json
from pathlib import Path

import pytest

from rotkit.hack.text_overrides import load_overrides, render_include, run
from rotkit.text_encoding import CODEMAP, unicode_to_game


def write_config(
    tmp_path: Path, content: str, name: str = "strings.text-override.toml"
) -> Path:
    path = tmp_path / name
    path.write_text(content, encoding="utf-8")
    return path


def test_font_encoding() -> None:
    assert unicode_to_game('ASCII "quotes" \\ @1\n') == b'ASCII "quotes" \\ @1\n'
    assert unicode_to_game("©¡¿ÁÄÈÉÓÚÜßàáâäçèéêëìíîïñòóôöùúûü") == bytes(
        (
            0x7B,
            0x7D,
            0x7E,
            0x80,
            0x83,
            0x87,
            0x88,
            0x91,
            0x97,
            0x98,
            0x99,
            0x9A,
            0x9B,
            0x9C,
            0x9E,
            0xA1,
            0xA2,
            0xA3,
            0xA4,
            0xA5,
            0xA6,
            0xA7,
            0xA8,
            0xA9,
            0xAA,
            0xAB,
            0xAC,
            0xAD,
            0xAF,
            0xB1,
            0xB2,
            0xB3,
            0xB4,
        )
    )
    assert len(CODEMAP) == len(set(CODEMAP.values()))


@pytest.mark.parametrize(
    "character", ("\u2014", "\u2603", "{", "}", "~", "|", "\t", "\r", "\0")
)
def test_unsupported_characters(character: str) -> None:
    with pytest.raises(ValueError, match=f"U\\+{ord(character):04X}"):
        unicode_to_game(character)


def test_id_first_translations(tmp_path: Path) -> None:
    source = write_config(
        tmp_path,
        '[TEXT_ID_FRODO]\nen = ""\nfr = "Frodon"\nde = "Präzision\\n@1"\n',
    )
    assert load_overrides([source]) == {
        ("TEXT_ID_FRODO", "en"): b"",
        ("TEXT_ID_FRODO", "fr"): b"Frodon",
        ("TEXT_ID_FRODO", "de"): b"Pr\x9ezision\n@1",
    }


def test_multiline_strings(tmp_path: Path) -> None:
    source = write_config(
        tmp_path,
        '[TEXT_ID_FRODO]\nde = """\nErste Zeile\nZweite Zeile"""\n'
        'en = """\nA long sentence \\\n    continued without a newline."""\n',
    )
    assert load_overrides([source]) == {
        ("TEXT_ID_FRODO", "de"): b"Erste Zeile\nZweite Zeile",
        ("TEXT_ID_FRODO", "en"): b"A long sentence continued without a newline.",
    }


@pytest.mark.parametrize(
    ("content", "message"),
    (
        ('[TEXT_ID_MISSING]\nde = "text"', "unknown text ID"),
        ('TEXT_ID_FRODO = "text"', "expected a table"),
        ('[TEXT_ID_FRODO]\npt = "text"', "unknown language"),
        ('[TEXT_ID_FRODO]\nDE = "text"', "unknown language"),
        ("[TEXT_ID_FRODO]\nde = 42", "expected a string"),
        ("[TEXT_ID_FRODO]\nde = true", "expected a string"),
        ('[TEXT_ID_FRODO]\nde = ["text"]', "expected a string"),
        ('[TEXT_ID_FRODO.de]\ntext = "text"', "expected a string"),
        ('[TEXT_ID_FRODO]\nde = "text"\nde = "again"', "overwrite a value"),
        ('[TEXT_ID_FRODO]\nde = "text"\n[TEXT_ID_FRODO]', "Cannot declare"),
        (
            '[TEXT_ID_FRODO]\nde = "\\u2603"',
            r"TEXT_ID_FRODO.de: unsupported character .*U\+2603",
        ),
        (
            '[TEXT_ID_FRODO]\nde = "\\u0000"',
            r"TEXT_ID_FRODO.de: unsupported character .*U\+0000",
        ),
    ),
)
def test_invalid_declarations(tmp_path: Path, content: str, message: str) -> None:
    source = write_config(tmp_path, content)
    with pytest.raises(ValueError, match=message) as error:
        load_overrides([source])
    assert str(source) in str(error.value)


def test_invalid_utf8(tmp_path: Path) -> None:
    source = tmp_path / "invalid.toml"
    source.write_bytes(b"\xff")
    with pytest.raises(ValueError, match="invalid.toml"):
        load_overrides([source])


def test_duplicate_across_files(tmp_path: Path) -> None:
    first = write_config(tmp_path, '[TEXT_ID_FRODO]\nde = "one"', "a.text-override.toml")
    second = write_config(tmp_path, '[TEXT_ID_FRODO]\nde = "two"', "b.text-override.toml")
    with pytest.raises(ValueError, match="duplicate override") as error:
        load_overrides([second, first])
    assert str(first) in str(error.value)
    assert str(second) in str(error.value)
    assert "TEXT_ID_FRODO.de" in str(error.value)


def test_disjoint_languages_across_files(tmp_path: Path) -> None:
    first = write_config(tmp_path, '[TEXT_ID_FRODO]\nde = "eins"', "a.text-override.toml")
    second = write_config(tmp_path, '[TEXT_ID_FRODO]\nen = "one"', "b.text-override.toml")
    assert load_overrides([first, second]) == {
        ("TEXT_ID_FRODO", "de"): b"eins",
        ("TEXT_ID_FRODO", "en"): b"one",
    }
    assert render_include(load_overrides([first, second])) == render_include(
        load_overrides([second, first])
    )


@pytest.mark.parametrize("length", (1023, 1024))
def test_encoded_length_limit(tmp_path: Path, length: int) -> None:
    source = write_config(
        tmp_path,
        "[TEXT_ID_FRODO]\nde = " + json.dumps("ä" * length, ensure_ascii=False),
    )
    if length == 1023:
        assert load_overrides([source])["TEXT_ID_FRODO", "de"] == b"\x9e" * length
    else:
        with pytest.raises(ValueError, match="1025 bytes.*maximum 1024"):
            load_overrides([source])


def test_empty_configuration(tmp_path: Path) -> None:
    source = write_config(tmp_path, "# No replacements.\n[TEXT_ID_FRODO]\n")
    assert load_overrides([source]) == {}
    assert load_overrides([]) == {}
    assert "TEXT_OVERRIDE(" not in render_include({})


def test_regeneration_tracks_added_and_removed_files(tmp_path: Path) -> None:
    source = tmp_path / "source"
    source.mkdir()
    out = tmp_path / "include/text_overrides.inc"
    run(source, out)
    assert "TEXT_OVERRIDE(" not in out.read_text()
    config = write_config(source, '[TEXT_ID_FRODO]\nde = "ä"')
    run(source, out)
    assert 'TEXT_OVERRIDE(LANGUAGE_DE, TEXT_ID_FRODO, "\\236")' in out.read_text()
    config.unlink()
    run(source, out)
    assert "TEXT_OVERRIDE(" not in out.read_text()


def test_invalid_input_keeps_previous_output(tmp_path: Path) -> None:
    out = tmp_path / "text_overrides.inc"
    out.write_text("previous output")
    write_config(tmp_path, "[TEXT_ID_FRODO]\nde = 42")
    with pytest.raises(ValueError, match="expected a string"):
        run(tmp_path, out)
    assert out.read_text() == "previous output"
