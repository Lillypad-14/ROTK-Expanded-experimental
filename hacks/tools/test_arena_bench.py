"""CPU sample attribution for native code copied into fast memory."""

import json
from pathlib import Path

import pytest

import arena_bench

from arena_bench import name_of


@pytest.mark.parametrize(
    ("pc", "expected"),
    [
        (0x03002C2F, "iwram:03002c00"),
        (0x03002C30, "iwram:actor render sort"),
        (0x03002D27, "iwram:actor render sort"),
        (0x03002D28, "iwram:03002d00"),
        (0x03002D2F, "iwram:03002d00"),
        (0x03002D30, "iwram:actor_checkCollisions"),
        (0x0300320B, "iwram:actor_checkCollisions"),
        (0x0300320C, "iwram:03003200"),
        (0x0300768F, "iwram:03007600"),
        (0x03007690, "iwram:sprite_findFreeObjTiles"),
        (0x0300771B, "iwram:sprite_findFreeObjTiles"),
        (0x0300771C, "iwram:03007700"),
        (0x02002001, "ewram:02002000"),
        (0x08000709, "game:actor_tickAllInList"),
        (0x09000001, "mod:modEntry"),
    ],
)
def test_cpu_sample_name(pc: int, expected: str) -> None:
    assert (
        name_of(pc, [0x08000708, 0x09000000], ["actor_tickAllInList", "modEntry"])
        == expected
    )


@pytest.mark.parametrize(
    ("sampling", "sampled"), [(False, False), (True, False), (True, True)]
)
@pytest.mark.parametrize("custom_paths", [False, True])
def test_completed_run_without_cpu_samples(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    sampling: bool,
    sampled: bool,
    custom_paths: bool,
) -> None:
    build = tmp_path / "build"
    build.mkdir()
    rom = tmp_path / "custom.gba" if custom_paths else build / "test_arena.gba"
    elf = tmp_path / "custom.elf" if custom_paths else build / "test_arena.elf"
    rom.write_bytes(bytes(24))
    elf.touch()
    emulator = tmp_path / "emulator.exe"
    emulator.touch()
    output = tmp_path / "report"

    class Process:
        def __init__(self, *args: object, **kwargs: object) -> None:
            samples = (
                "ARENA_PC 03002c30 03002d30\nARENA_PC_DROPPED 1\n"
                if sampled
                else "ARENA_PC_UNAVAILABLE 1\n"
            )
            (output / "mgba.log").write_text(samples + "ARENA_BENCH 5 10 0 2 20 1\n")

        def terminate(self) -> None:
            pass

        def wait(self, timeout: int) -> int:
            return 0

    monkeypatch.setattr(arena_bench, "ROOT", tmp_path)
    monkeypatch.setattr(arena_bench, "config_offset", lambda *args: 0)
    monkeypatch.setattr(arena_bench, "symbol_table", lambda *args: ([], []))
    monkeypatch.setattr(arena_bench.subprocess, "Popen", Process)
    monkeypatch.setattr(arena_bench.time, "sleep", lambda duration: None)
    arguments = ["arena_bench", "--emulator", str(emulator), "--output", str(output)]
    if custom_paths:
        arguments += ["--rom", str(rom), "--elf", str(elf)]
    if not sampling:
        arguments.append("--no-samples")
    monkeypatch.setattr("sys.argv", arguments)

    if sampling and not sampled:
        with pytest.raises(SystemExit, match="CPU sampling returned no samples"):
            arena_bench.main()
    else:
        arena_bench.main()

    report = json.loads((output / "result.json").read_text())
    assert report["ticks"] == 5
    assert report["vblanksPerTick"] == 2.0
    assert report["samplingStatus"] == (
        "ok" if sampled else "no_samples" if sampling else "disabled"
    )
    if sampled:
        assert report["droppedSamplePercent"] == 33.33
        assert report["functions"] == [
            ["iwram:actor render sort", 50.0],
            ["iwram:actor_checkCollisions", 50.0],
        ]
    assert report["sourceRomSha256"] != report["arenaRomSha256"]
