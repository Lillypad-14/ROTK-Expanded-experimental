"""Selected-patch discovery and shared converter/diff inputs."""

from pathlib import Path

import pytest
from typer.testing import CliRunner

from rotkit.analysis import text_override_diff
from rotkit.cli import app
from rotkit.hack import text_overrides


@pytest.fixture
def patch_tree(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    root = tmp_path / "patches"
    for patch in ("text_overrides", "balance", "disabled", "plain"):
        (root / patch).mkdir(parents=True)
    (root / "text_overrides/strings.text-override.toml").write_text(
        '[TEXT_ID_FRODO]\nen = "Frodo"\n'
    )
    (root / "balance/strings.text-override.toml").write_text(
        '[TEXT_ID_FRODO]\nfr = "Frodon"\n'
    )
    (root / "disabled/strings.text-override.toml").write_text("invalid TOML")
    (root / "balance/settings.toml").write_text("not a text override")
    config = tmp_path / "patches.cfg"
    config.write_text("# Default patches\ntext_overrides # shared decoder\n")
    monkeypatch.setattr(text_overrides, "PATCH_ROOT", root)
    monkeypatch.setattr(text_overrides, "PATCH_CONFIG", config)
    monkeypatch.delenv("HACKS", raising=False)
    return root


@pytest.mark.parametrize(
    ("selection", "owners"),
    (
        (None, ["text_overrides"]),
        ("@cfg", ["text_overrides"]),
        ("@cfg balance", ["balance", "text_overrides"]),
        ("balance text_overrides balance @cfg", ["balance", "text_overrides"]),
        ("", []),
        ("plain", []),
    ),
)
def test_selected_sources(
    patch_tree: Path, selection: str | None, owners: list[str]
) -> None:
    paths = text_overrides.discover_sources(patches=selection)
    assert paths == [
        patch_tree / owner / "strings.text-override.toml" for owner in owners
    ]
    text_overrides.load_overrides(paths)


def test_environment_selection(
    patch_tree: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setenv("HACKS", "@cfg balance")
    assert len(text_overrides.discover_sources()) == 2
    assert len(text_overrides.discover_sources(patches="text_overrides")) == 1
    monkeypatch.setenv("HACKS", "")
    assert text_overrides.discover_sources() == []


def test_unknown_patch(patch_tree: Path) -> None:
    with pytest.raises(ValueError, match="unknown patches: missing"):
        text_overrides.discover_sources(patches="@cfg missing")


def test_missing_decoder_preserves_output(patch_tree: Path, tmp_path: Path) -> None:
    out = tmp_path / "text_overrides.inc"
    out.write_text("previous output")
    with pytest.raises(ValueError, match="require the text_overrides patch") as error:
        text_overrides.run(out=out, patches="balance")
    assert str(patch_tree / "balance/strings.text-override.toml") in str(error.value)
    assert out.read_text() == "previous output"


@pytest.mark.parametrize("replacement", ("Frodo", "Balanced Frodo"))
def test_cross_patch_duplicates_preserve_output(
    patch_tree: Path, tmp_path: Path, replacement: str
) -> None:
    conflict = patch_tree / "balance/strings.text-override.toml"
    conflict.write_text(f'[TEXT_ID_FRODO]\nen = "{replacement}"\n')
    out = tmp_path / "text_overrides.inc"
    out.write_text("previous output")
    with pytest.raises(ValueError, match="duplicate override") as error:
        text_overrides.run(out=out, patches="@cfg balance")
    assert str(conflict) in str(error.value)
    original = patch_tree / "text_overrides/strings.text-override.toml"
    assert str(original) in str(error.value)
    assert "TEXT_ID_FRODO.en" in str(error.value)
    assert out.read_text() == "previous output"


def test_selection_and_file_changes_regenerate(patch_tree: Path, tmp_path: Path) -> None:
    out = tmp_path / "text_overrides.inc"
    text_overrides.run(out=out, patches="@cfg balance")
    assert "LANGUAGE_FR" in out.read_text()
    text_overrides.run(out=out, patches="@cfg")
    assert "LANGUAGE_FR" not in out.read_text()
    extra = patch_tree / "text_overrides/extra.text-override.toml"
    extra.write_text('[TEXT_ID_FRODO]\nde = "Frodo"\n')
    text_overrides.run(out=out, patches="@cfg")
    assert "LANGUAGE_DE" in out.read_text()
    extra.unlink()
    text_overrides.run(out=out, patches="@cfg")
    assert "LANGUAGE_DE" not in out.read_text()
    text_overrides.run(out=out, patches="")
    assert "TEXT_OVERRIDE(" not in out.read_text()


def test_explicit_source_filters_suffix(patch_tree: Path) -> None:
    source = patch_tree / "balance"
    assert text_overrides.discover_sources(source) == [
        source / "strings.text-override.toml"
    ]
    with pytest.raises(ValueError, match="use either --source or --patches"):
        text_overrides.discover_sources(source, "@cfg")


def test_missing_source(tmp_path: Path) -> None:
    with pytest.raises(ValueError, match="override directory does not exist"):
        text_overrides.discover_sources(tmp_path / "missing")


@pytest.mark.parametrize(
    "command", (["hack", "text-overrides"], ["analysis", "decode-text", "diff"])
)
@pytest.mark.parametrize("selection", ("@cfg", "@cfg balance", ""))
def test_converter_and_diff_share_selection(
    patch_tree: Path,
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    command: list[str],
    selection: str,
) -> None:
    monkeypatch.setattr(text_override_diff, "load_rom", lambda: b"ROM")
    monkeypatch.setattr(
        text_override_diff,
        "render_diff",
        lambda rom, overrides, *args: text_overrides.render_include(overrides),
    )
    out = tmp_path / "output.txt"
    result = CliRunner().invoke(
        app, [*command, "--patches", selection, "--out", str(out)]
    )
    assert result.exit_code == 0, result.output
    expected = text_overrides.load_overrides(
        text_overrides.discover_sources(patches=selection)
    )
    assert out.read_text() == text_overrides.render_include(expected)
