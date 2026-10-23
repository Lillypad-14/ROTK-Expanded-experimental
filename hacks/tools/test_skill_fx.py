"""Importer regressions for palette fidelity, native frame data and invalid assets."""

from copy import deepcopy
import json
from pathlib import Path
import re
import shutil
import subprocess

from PIL import Image
import pytest

from skill_fx import compile_sheet, generate
from rotkit.spritegfx import parse_frame_set, decode_frame_tiles


def fixture(tmp_path: Path) -> tuple[dict, dict]:
    image = Image.new("RGBA", (16, 8), (0, 0, 0, 0))
    image.putpixel((0, 0), (255, 0, 0, 255))
    image.putpixel((8, 0), (0, 0, 0, 255))
    image.save(tmp_path / "sheet.png")
    asset = dict(key="test", id=0, sheet="sheet.png", width=8, height=8, frameTicks=3)
    document = dict(
        version=1,
        assets=[asset],
        effects=[dict(key="test", id=0, name="Test", layers=[dict(asset="test")])],
    )
    return asset, document


def blob(code: str, name: str) -> bytes:
    data = re.search(name + r"\[\].*?=\s*\{(.*?)\};", code, re.S)[1]
    return bytes(int(value, 16) for value in re.findall(r"0x([0-9a-f]+)", data))


def test_native_roundtrip_and_preview(tmp_path: Path) -> None:
    asset, document = fixture(tmp_path)
    compiled = compile_sheet(asset, tmp_path)
    frames = parse_frame_set(
        blob(compiled["code"], "SkillFxAsset_testFrames"), 0x08000000
    )
    tiles = blob(compiled["code"], "SkillFxAsset_testTiles")
    # Two drawn frames plus the end frame that repeats the last one.
    assert len(frames.frames) == 3 and frames.max_frame_tile_bytes == 32
    assert frames.frames[2].tile_offset == frames.frames[1].tile_offset
    assert frames.frames[0].oam[0].x == -4 and frames.frames[0].oam[0].y == -8
    # Opaque black must remain visible, distinct from transparent index zero.
    first = decode_frame_tiles(tiles, frames.frames[0])
    second = decode_frame_tiles(tiles, frames.frames[1])
    assert first[0] & 15 and second[0] & 15 and first[0] != second[0]
    assert first[1:] == bytes(31)
    manifest = tmp_path / "effects.json"
    manifest.write_text(json.dumps(document))
    generate(manifest, tmp_path / "generated", tmp_path / "preview")
    assert "frameTicks=3" in (tmp_path / "generated/skill_fx_assets.inc").read_text()
    with Image.open(tmp_path / "preview/test.gif") as preview:
        assert preview.n_frames == 2
    with Image.open(tmp_path / "preview/test.png") as preview:
        assert preview.size == (16, 8)


@pytest.mark.parametrize(
    "field,value,match",
    [
        ("width", 9, "multiples of 8"),
        ("width", 128, "tile buffer"),
        ("frameTicks", 0, "frameTicks"),
        ("id", 32, "palette ID"),
        ("sheet", "../escape.png", "inside"),
        ("anchor", [256, 0], "anchor"),
    ],
)
def test_invalid_asset(tmp_path: Path, field: str, value: object, match: str) -> None:
    asset, _ = fixture(tmp_path)
    if field == "width" and value == 128:
        asset["height"] = 64
    asset[field] = value
    with pytest.raises(ValueError, match=match):
        compile_sheet(asset, tmp_path)


def test_palette_and_alpha_limits(tmp_path: Path) -> None:
    asset, _ = fixture(tmp_path)
    image = Image.new("RGBA", (8, 8))
    image.putpixel((0, 0), (255, 0, 0, 128))
    image.save(tmp_path / "sheet.png")
    with pytest.raises(ValueError, match="partial transparency"):
        compile_sheet(asset, tmp_path)
    for i in range(16):
        image.putpixel((i % 8, i // 8), (i * 16, 0, 0, 255))
    image.save(tmp_path / "sheet.png")
    with pytest.raises(ValueError, match="15 opaque"):
        compile_sheet(asset, tmp_path)


@pytest.mark.parametrize(
    "layer,match",
    [
        ({"asset": "missing"}, "unknown sprite"),
        ({"shape": "typo", "color": "white"}, "unknown shape"),
        ({"asset": "test", "duration": 30}, "lifetimes"),
        ({"asset": "test", "attachment": "typo"}, "attachment"),
        ({"asset": "test", "shape": "spark"}, "choose asset"),
    ],
)
def test_invalid_recipe(tmp_path: Path, layer: dict, match: str) -> None:
    _, document = fixture(tmp_path)
    document["effects"][0]["layers"] = [layer]
    manifest = tmp_path / "effects.json"
    manifest.write_text(json.dumps(document))
    with pytest.raises(ValueError, match=match):
        generate(manifest, tmp_path / "output", tmp_path / "preview")


def test_duplicate_palette_identity(tmp_path: Path) -> None:
    _, document = fixture(tmp_path)
    second = deepcopy(document["assets"][0])
    second["key"] = "other"
    second["sheet"] = "different.png"
    image = Image.new("RGBA", (16, 8), (0, 0, 0, 0))
    image.putpixel((0, 0), (0, 255, 0, 255))
    image.save(tmp_path / "different.png")
    document["assets"].append(second)
    manifest = tmp_path / "effects.json"
    manifest.write_text(json.dumps(document))
    with pytest.raises(ValueError, match="duplicate asset"):
        generate(manifest, tmp_path / "output", tmp_path / "preview")


def test_shared_palette_identity(tmp_path: Path) -> None:
    _, document = fixture(tmp_path)
    second = dict(document["assets"][0], key="other")
    document["assets"].append(second)
    manifest = tmp_path / "effects.json"
    manifest.write_text(json.dumps(document))
    generate(manifest, tmp_path / "output", tmp_path / "preview")
    assert (
        "SkillFxAsset_otherPalette"
        in (tmp_path / "output/skill_fx_assets.inc").read_text()
    )


def test_runtime(tmp_path: Path) -> None:
    root = Path(__file__).resolve().parents[2]
    compiler = shutil.which("clang") or shutil.which("cc")
    assert compiler, "Install clang or cc for the effect runtime regressions"
    generate(
        root / "hacks/content/skill_fx.json", tmp_path / "include", tmp_path / "preview"
    )
    executable = tmp_path / "fx-tests.exe"
    subprocess.run(
        [
            compiler,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            f"-I{tmp_path / 'include'}",
            *(
                f"-I{root / path}"
                for path in ("include", "carved/include", "build/include", "hacks")
            ),
            str(root / "hacks/tests/skill_fx.c"),
            str(root / "hacks/lib/skill_fx.c"),
            "-o",
            str(executable),
        ],
        check=True,
    )
    subprocess.run([str(executable)], check=True)


def test_shared_palette_strip(tmp_path: Path) -> None:
    asset, document = fixture(tmp_path)
    strip = Image.new("RGBA", (3, 1))
    for x, color in enumerate(((0, 0, 255), (0, 0, 0), (255, 0, 0))):
        strip.putpixel((x, 0), (*color, 255))
    strip.save(tmp_path / "strip.png")
    first = dict(asset, palette="strip.png")
    second = dict(first, key="other")
    compiled = compile_sheet(first, tmp_path)
    # Strip order decides the indexes, unused strip colors stay in the palette.
    assert compiled["palette"][1:4] == [0x7C00, 0x0000, 0x001F]
    document["assets"] = [first, second]
    manifest = tmp_path / "effects.json"
    manifest.write_text(json.dumps(document))
    generate(manifest, tmp_path / "generated", tmp_path / "preview")
    document["assets"] = [asset, second]
    manifest.write_text(json.dumps(document))
    with pytest.raises(ValueError, match="palette ID"):
        generate(manifest, tmp_path / "generated", tmp_path / "preview")
    strip.putpixel((2, 0), (0, 255, 0, 255))
    strip.save(tmp_path / "strip.png")
    with pytest.raises(ValueError, match="outside its palette"):
        compile_sheet(first, tmp_path)
