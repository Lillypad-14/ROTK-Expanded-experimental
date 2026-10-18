"""Review TOML replacements against the original localized ROM strings."""

from collections.abc import Sequence
from difflib import unified_diff
from pathlib import Path
from typing import Annotated

import typer

from rotkit import textdb
from rotkit.analysis.decode_text import LANG_TAGS, render, resolve_ids
from rotkit.cheaders import extract_enum
from rotkit.hack.text_overrides import discover_sources, load_overrides
from rotkit.rom import load_rom


def language_lines(language: str, raw: bytes) -> list[str]:
    text = render(raw).split("\n")
    return [f"{language}: {text[0]}\n", *(f"    {line}\n" for line in text[1:])]


def render_diff(
    rom: bytes,
    overrides: dict[tuple[str, str], bytes],
    selected_ids: set[int] | None = None,
    languages: Sequence[str] | None = None,
) -> str:
    """Diff one labeled translation group per ID with all languages as context."""
    if languages is None:
        languages = LANG_TAGS
    ids = extract_enum("carved/include/text_ids.h", "TextId")
    packages = {
        language: textdb.pkg_for(rom, LANG_TAGS.index(language))
        for language in languages
    }
    lines = []
    for symbol in sorted({symbol for symbol, _ in overrides}, key=ids.__getitem__):
        text_id = ids[symbol]
        if selected_ids is not None and text_id not in selected_ids:
            continue
        before = []
        after = []
        for language in languages:
            package = packages[language]
            if text_id >= textdb.string_count(rom, package):
                raise ValueError(
                    f"{symbol} ({text_id}): no {language} string in the ROM"
                )
            old = textdb.decode(rom, package, text_id)[0]
            new = overrides.get((symbol, language), old)
            before.extend(language_lines(language, old))
            after.extend(language_lines(language, new))
        path = f"{text_id}-{symbol}.txt"
        lines.extend(
            unified_diff(
                before,
                after,
                fromfile=f"rom/{path}",
                tofile=f"overrides/{path}",
                n=max(len(before), len(after)),
            )
        )
    return "".join(lines)


def run(
    ids: Annotated[
        list[str] | None,
        typer.Argument(help="limit to TEXT_ID_... names, decimal IDs, or 0x hex IDs"),
    ] = None,
    language: Annotated[
        list[str] | None,
        typer.Option(
            "--language", "-l", help="language tag (repeatable, default: all five)"
        ),
    ] = None,
    out: Annotated[
        Path | None,
        typer.Option(
            "--out", "-o", help="write a UTF-8 unified diff instead of stdout"
        ),
    ] = None,
    source: Annotated[
        Path | None,
        typer.Option(exists=True, file_okay=False, help="use only this override directory"),
    ] = None,
    patches: Annotated[
        str | None,
        typer.Option(help="patch selection, e.g. '@cfg balance' (default: HACKS or @cfg)"),
    ] = None,
) -> None:
    """Emit a unified diff of original ROM strings versus TOML overrides."""
    selected_ids = set(resolve_ids(ids)) if ids else None
    requested_languages = set(language) if language is not None else set(LANG_TAGS)
    unknown = requested_languages - set(LANG_TAGS)
    if unknown:
        raise typer.BadParameter(
            f"unknown language: {', '.join(sorted(unknown))} (use {', '.join(LANG_TAGS)})",
            param_hint="--language",
        )
    languages = [tag for tag in LANG_TAGS if tag in requested_languages]
    overrides = load_overrides(discover_sources(source, patches))
    diff = render_diff(load_rom(), overrides, selected_ids, languages)
    if out is None:
        print(diff, end="")
    else:
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(diff, encoding="utf-8")
        typer.echo(f"Wrote {out}", err=True)
