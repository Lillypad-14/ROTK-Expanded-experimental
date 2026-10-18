"""Symbolic text IDs and ROM-free override review reports."""

from pathlib import Path

import pytest
import typer
from typer.testing import CliRunner

from rotkit import textdb
from rotkit.analysis import decode_text, text_override_diff
from rotkit.cli import app


@pytest.mark.parametrize(
    ("values", "expected"),
    (
        (["370"], [370]),
        (["0x172"], [370]),
        (["TEXT_ID_STAT_CORRUPTION_RESISTANCE"], [370]),
        (["TEXT_ID_FRODO", "370", "0x234"], [19, 370, 564]),
    ),
)
def test_resolve_ids(values: list[str], expected: list[int]) -> None:
    assert decode_text.resolve_ids(values) == expected


def test_numeric_ids_do_not_require_header_parsing(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    def unexpected_parse(*args: object) -> None:
        pytest.fail("numeric IDs should not require libclang")

    monkeypatch.setattr(decode_text, "extract_enum", unexpected_parse)
    assert decode_text.resolve_ids(["19", "0x172"]) == [19, 370]


@pytest.mark.parametrize("value", ("TEXT_ID_MISSING", "not-an-id", "0xZZ", "-1"))
def test_invalid_ids(value: str) -> None:
    with pytest.raises(typer.BadParameter):
        decode_text.resolve_ids([value])


@pytest.fixture
def originals(monkeypatch: pytest.MonkeyPatch) -> dict[tuple[int, int], bytes]:
    values = {
        (language, text_id): b"Original"
        for language in range(5)
        for text_id in (19, 370, 564)
    }

    def decode(rom: bytes, package: int, text_id: int) -> tuple[bytes, int]:
        return values[package, text_id], 3

    monkeypatch.setattr(textdb, "pkg_for", lambda rom, language: language)
    monkeypatch.setattr(textdb, "string_count", lambda rom, package: 565)
    monkeypatch.setattr(textdb, "decode", decode)
    monkeypatch.setattr(decode_text, "load_rom", lambda: b"ROM")
    monkeypatch.setattr(text_override_diff, "load_rom", lambda: b"ROM")
    return values


@pytest.mark.parametrize(
    "value", ("TEXT_ID_STAT_CORRUPTION_RESISTANCE", "370", "0x172")
)
def test_show_symbolic_id(originals: dict[tuple[int, int], bytes], value: str) -> None:
    originals[2, 370] = b"% Resistenz geg. Verf\xb4hrung"
    result = CliRunner().invoke(app, ["analysis", "decode-text", "show", value])
    assert result.exit_code == 0, result.output
    assert "id 370 - TEXT_ID_STAT_CORRUPTION_RESISTANCE:" in result.stdout
    assert "de:" in result.stdout
    assert (
        "de: '% Resistenz geg. Verführung' (size 27B, 3B compressed)" in result.stdout
    )
    for language in decode_text.LANG_TAGS:
        assert f"  {language}:" in result.stdout


@pytest.mark.parametrize("command", ("show", "diff"))
def test_unknown_id_cli(command: str) -> None:
    result = CliRunner().invoke(
        app, ["analysis", "decode-text", command, "TEXT_ID_MISSING"]
    )
    assert result.exit_code == 2
    assert "unknown text ID" in result.output
    assert "Traceback" not in result.output


def test_show_requires_id() -> None:
    result = CliRunner().invoke(app, ["analysis", "decode-text", "show"])
    assert result.exit_code == 2
    assert "at least one string id" in result.output


def test_diff_groups_ids_and_languages(
    originals: dict[tuple[int, int], bytes],
) -> None:
    originals[0, 19] = b"Frodo"
    diff = text_override_diff.render_diff(
        b"ROM",
        {
            ("TEXT_ID_STAT_CORRUPTION_RESISTANCE", "de"): b"% Verderbnisresistenz",
            ("TEXT_ID_FRODO", "fr"): b"Frodon",
            ("TEXT_ID_FRODO", "en"): b"Frodo",
        },
    )
    assert diff.count("--- rom/") == 2
    assert diff.index("19-TEXT_ID_FRODO.txt") < diff.index(
        "370-TEXT_ID_STAT_CORRUPTION_RESISTANCE.txt"
    )
    assert " en: Frodo\n-fr: Original\n+fr: Frodon\n" in diff
    assert " de: Original\n it: Original\n es: Original\n" in diff
    assert "```" not in diff


def test_diff_preserves_empty_strings_and_whitespace(
    originals: dict[tuple[int, int], bytes],
) -> None:
    originals[2, 19] = b"Pr\x9ezision\n@1\n"
    diff = text_override_diff.render_diff(
        b"ROM", {("TEXT_ID_FRODO", "de"): b"", ("TEXT_ID_FRODO", "fr"): b"@1 "}
    )
    assert "-de: Präzision\n-    @1\n-    \n" in diff
    assert "+de: \n" in diff
    assert "+fr: @1 \n" in diff


def test_empty_replacement(originals: dict[tuple[int, int], bytes]) -> None:
    diff = text_override_diff.render_diff(b"ROM", {("TEXT_ID_FRODO", "en"): b""})
    assert "@@ -1,5 +1,5 @@\n-en: Original\n+en: \n" in diff


def test_multiline_diff(originals: dict[tuple[int, int], bytes]) -> None:
    originals[0, 19] = b"First\nOld\nLast"
    diff = text_override_diff.render_diff(
        b"ROM", {("TEXT_ID_FRODO", "en"): b"First\nNew\nLast"}, languages=["en"]
    )
    assert diff == (
        "--- rom/19-TEXT_ID_FRODO.txt\n"
        "+++ overrides/19-TEXT_ID_FRODO.txt\n"
        "@@ -1,3 +1,3 @@\n"
        " en: First\n-    Old\n+    New\n     Last\n"
    )


def test_newline_only_change_is_visible(
    originals: dict[tuple[int, int], bytes],
) -> None:
    originals[0, 19] = b"Frodo"
    diff = text_override_diff.render_diff(
        b"ROM", {("TEXT_ID_FRODO", "en"): b"Frodo\n"}, languages=["en"]
    )
    assert "@@ -1 +1,2 @@\n en: Frodo\n+    \n" in diff


def test_full_language_context_is_not_truncated(
    originals: dict[tuple[int, int], bytes],
) -> None:
    originals[0, 19] = b"Original\n" + b"Continuation\n" * 12
    diff = text_override_diff.render_diff(
        b"ROM", {("TEXT_ID_FRODO", "en"): b"Changed\n" + b"Continuation\n" * 12}
    )
    assert diff.count("\n@@ ") == 1
    assert "-en: Original\n+en: Changed\n" in diff
    for language in ("fr", "de", "it", "es"):
        assert f" {language}: Original\n" in diff


def test_diff_filters(originals: dict[tuple[int, int], bytes]) -> None:
    diff = text_override_diff.render_diff(
        b"ROM",
        {
            ("TEXT_ID_FRODO", "en"): b"Frodo",
            ("TEXT_ID_FRODO", "de"): b"Frodo",
            ("TEXT_ID_STAT_CORRUPTION_RESISTANCE", "de"): b"% Verderbnisresistenz",
        },
        {19},
        ["de"],
    )
    assert diff.count("--- rom/") == 1
    assert "19-TEXT_ID_FRODO.txt" in diff
    assert "-de: Original\n+de: Frodo\n" in diff
    assert "en:" not in diff
    assert "TEXT_ID_STAT_CORRUPTION_RESISTANCE" not in diff
    assert (
        text_override_diff.render_diff(
            b"ROM", {("TEXT_ID_FRODO", "en"): b"Frodo"}, languages=["de"]
        )
        == ""
    )


@pytest.mark.parametrize("overrides", ({}, {("TEXT_ID_FRODO", "en"): b"Original"}))
def test_no_changes(
    originals: dict[tuple[int, int], bytes], overrides: dict[tuple[str, str], bytes]
) -> None:
    assert text_override_diff.render_diff(b"ROM", overrides) == ""


def test_rom_missing_id(
    originals: dict[tuple[int, int], bytes], monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(textdb, "string_count", lambda rom, package: 10)
    with pytest.raises(ValueError, match="TEXT_ID_FRODO.*no de string"):
        text_override_diff.render_diff(
            b"ROM", {("TEXT_ID_FRODO", "de"): b"Frodo"}, languages=["de"]
        )


def test_diff_cli_stdout_and_file(
    originals: dict[tuple[int, int], bytes], tmp_path: Path
) -> None:
    source = tmp_path / "source"
    source.mkdir()
    (source / "strings.text-override.toml").write_text(
        '[TEXT_ID_FRODO]\nen = "Frodo"\nfr = "Frodon"\nde = "Präzision"\n',
        encoding="utf-8",
    )
    command = ["analysis", "decode-text", "diff", "--source", str(source)]
    result = CliRunner().invoke(
        app, [*command, "TEXT_ID_FRODO", "-l", "de", "-l", "fr"]
    )
    assert result.exit_code == 0, result.output
    assert result.stdout.count("--- rom/") == 1
    assert "Präzision" in result.stdout
    assert "en:" not in result.stdout
    assert result.stdout.index("fr:") < result.stdout.index("de:")
    out = tmp_path / "reports/text.diff"
    saved = CliRunner().invoke(
        app, [*command, "19", "-l", "de", "-l", "fr", "-o", str(out)]
    )
    assert saved.exit_code == 0, saved.output
    assert saved.stdout == ""
    assert out.read_text(encoding="utf-8") == result.stdout


def test_invalid_language_cli() -> None:
    result = CliRunner().invoke(app, ["analysis", "decode-text", "diff", "-l", "pt"])
    assert result.exit_code == 2
    assert "unknown language: pt" in result.output
    assert "Traceback" not in result.output
