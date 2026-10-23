"""Exercise the FX helpers against host-backed native palette tables."""

from pathlib import Path
import shutil
import subprocess


def test_fx_resources(tmp_path: Path) -> None:
    import pytest
    from ground_shockwave import source as shockwave_source

    root = Path(__file__).resolve().parents[2]
    compiler = shutil.which("clang") or shutil.which("cc")
    assert compiler, "Install clang or cc for FX resource regressions"
    rom_path = root / "rotk.gba"
    if not rom_path.exists():
        pytest.skip("The ground shockwave is generated from the base ROM")
    shockwave = tmp_path / "fx_shockwave.c"
    shockwave.write_text(shockwave_source(rom_path.read_bytes()))
    source = (root / "hacks/lib/fx_kit.c").read_text()
    # Only hardware addresses change. Compile the production allocation and cleanup paths.
    source = (
        '#include "types.h"\nextern u8 testTags[16], testRefs[16];\n'
        + source.replace("((u8 *)0x030053d0)", "testTags").replace(
            "((u8 *)0x030053e0)", "testRefs"
        )
    )
    translated = tmp_path / "fx_kit.c"
    translated.write_text(source)
    executable = tmp_path / "fx-resources.exe"
    subprocess.run(
        [
            compiler,
            "-std=gnu11",
            "-Wall",
            "-Wextra",
            "-Werror",
            *(
                f"-I{root / path}"
                for path in (
                    "include",
                    "carved/include",
                    "build/include",
                    "hacks",
                    "hacks/lib",
                )
            ),
            str(translated),
            str(root / "hacks/tests/fx_resources.c"),
            str(shockwave),
            "-o",
            str(executable),
        ],
        check=True,
    )
    subprocess.run([str(executable)], check=True)


def test_native_aura_table_bounds() -> None:
    import re
    import struct
    import pytest

    root = Path(__file__).resolve().parents[2]
    rom_path = root / "rotk.gba"
    if not rom_path.exists():
        pytest.skip("Native effect table check needs the base ROM")
    rom = rom_path.read_bytes()
    table = 0x082877FC
    descriptor_start = next(
        int(line.split()[0], 16)
        for line in (root / "config/data.cfg").read_text().splitlines()
        if len(line.split()) >= 2 and line.split()[1] == "FxDescriptors"
    )
    count = (descriptor_start - table) // (4 * 16)
    declared = re.search(
        r"FX_AURA_COUNT\s*=\s*(\d+)", (root / "hacks/lib/fx_kit.h").read_text()
    )
    assert declared and int(declared.group(1)) == count == 14
    for aura in range(count):
        for phase in range(3):
            tiles, frames = struct.unpack_from(
                "<II", rom, table - 0x08000000 + aura * 64 + phase * 16
            )
            assert 0x08000000 <= tiles < 0x08000000 + len(rom)
            assert 0x08000000 <= frames < 0x08000000 + len(rom)
    # Row 14 aliases the one-shot table. Its next phase is not an animation descriptor.
    invalid_tiles = struct.unpack_from("<I", rom, descriptor_start - 0x08000000 + 16)[0]
    assert not 0x08000000 <= invalid_tiles < 0x08000000 + len(rom)


def test_complete_shockwave() -> None:
    import struct
    import pytest
    from ground_shockwave import combine, source
    from rotkit.spritegfx import decode_frame_tiles, parse_frame_set

    root = Path(__file__).resolve().parents[2]
    rom_path = root / "rotk.gba"
    if not rom_path.exists():
        pytest.skip("Stock shockwave generation needs the base ROM")
    rom = rom_path.read_bytes()
    tiles, frames = combine(rom)
    combined = parse_frame_set(frames, 0x08000000)
    halves = []
    for shape in (21, 20):
        address, frame_address = struct.unpack_from("<II", rom, 0x287B7C + shape * 20)
        halves.append((address - 0x08000000, parse_frame_set(rom, frame_address)))
    assert combined.frame_count == 17
    assert combined.max_frame_tile_bytes == 896
    for index, frame in enumerate(combined.frames):
        top = halves[0][1].frames[index]
        bottom = halves[1][1].frames[index]
        expected = b"".join(
            decode_frame_tiles(rom[address:], fs.frames[index])
            for address, fs in halves
        )
        assert decode_frame_tiles(tiles, frame) == expected
        assert frame.oam[: len(top.oam)] == top.oam
        assert len(frame.oam) == len(top.oam) + len(bottom.oam)
        for obj, native in zip(frame.oam[len(top.oam) :], bottom.oam):
            assert (obj.x, obj.y, obj.size) == (native.x, native.y, native.size)
            assert obj.tile_offset == native.tile_offset + top.tile_bytes // 32
    generated = root / "build/generated/fx_shockwave.c"
    if generated.exists():
        assert generated.read_text() == source(rom)
