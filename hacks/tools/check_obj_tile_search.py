"""Compare the optimized ARM tile search with the original ROM under Unicorn.

Run: uv run --with unicorn python hacks/tools/check_obj_tile_search.py
Requires armips on PATH and the original gitignored rotk.gba.
"""

import argparse
import hashlib
from pathlib import Path
import random
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
ROM_ADDRESS = 0x0800E17C
CODE_SIZE = 140
CODE = 0x03007690
STOP = 0x03007F00
STACK = 0x03007E00
BITMAP = 0x02000000


def assemble(armips: str) -> bytes:
    """Assemble the same source included by the hack build."""
    with tempfile.TemporaryDirectory() as directory:
        folder = Path(directory)
        binary = folder / "search.bin"
        source = folder / "search.asm"
        patch = ROOT / "hacks/patches/fast_obj_tiles/fast_obj_tiles.asm"
        source.write_text(
            f'.gba\n.create "{binary.as_posix()}", {ROM_ADDRESS:#x}\n'
            f".definelabel sprite_findFreeObjTiles, {ROM_ADDRESS:#x}\n"
            f'.include "{patch.as_posix()}"\n.close\n'
        )
        subprocess.run(
            [armips, str(source)], check=True, capture_output=True, text=True
        )
        code = binary.read_bytes()
    if len(code) != CODE_SIZE:
        raise AssertionError(f"search occupies {len(code)} bytes, expected {CODE_SIZE}")
    return code


def check(rom: bytes, optimized: bytes, random_cases: int, seed: int) -> int:
    """Check returns, interworking, preserved registers, stack and bitmap writes."""
    from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM
    from unicorn.arm_const import (
        UC_CPU_ARM_TI925T,
        UC_ARM_REG_CPSR,
        UC_ARM_REG_LR,
        UC_ARM_REG_PC,
        UC_ARM_REG_SP,
        UC_ARM_REG_R0,
        UC_ARM_REG_R1,
        UC_ARM_REG_R2,
        UC_ARM_REG_R4,
        UC_ARM_REG_R5,
        UC_ARM_REG_R6,
        UC_ARM_REG_R7,
        UC_ARM_REG_R8,
        UC_ARM_REG_R9,
        UC_ARM_REG_R10,
        UC_ARM_REG_R11,
    )

    offset = ROM_ADDRESS - 0x08000000
    machines = []
    for code in (rom[offset : offset + CODE_SIZE], optimized):
        machine = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        machine.ctl_set_cpu_model(UC_CPU_ARM_TI925T)
        machine.mem_map(0x03000000, 0x10000)
        machine.mem_map(BITMAP, 0x1000)
        machine.mem_write(CODE, code)
        machines.append(machine)
    preserved = (
        UC_ARM_REG_R4,
        UC_ARM_REG_R5,
        UC_ARM_REG_R6,
        UC_ARM_REG_R7,
        UC_ARM_REG_R8,
        UC_ARM_REG_R9,
        UC_ARM_REG_R10,
        UC_ARM_REG_R11,
    )
    register_values = [0x11223340 + index for index in range(len(preserved))]
    completed = 0

    def compare(bitmap: bytes, tail: bytes, count: int, first: int) -> None:
        nonlocal completed
        results = []
        thumb_return = completed & 1
        data = bitmap + tail
        for machine in machines:
            machine.mem_write(BITMAP, data)
            machine.mem_write(STACK - 32, bytes([0xA5]) * 64)
            machine.reg_write(UC_ARM_REG_CPSR, 0x1F)
            machine.reg_write(UC_ARM_REG_SP, STACK)
            machine.reg_write(UC_ARM_REG_LR, STOP | thumb_return)
            for register, value in zip(preserved, register_values):
                machine.reg_write(register, value)
            machine.reg_write(UC_ARM_REG_R0, BITMAP)
            machine.reg_write(UC_ARM_REG_R1, count)
            machine.reg_write(UC_ARM_REG_R2, first)
            machine.emu_start(CODE, STOP, count=100000)
            assert machine.reg_read(UC_ARM_REG_PC) == STOP, "did not return"
            assert machine.reg_read(UC_ARM_REG_SP) == STACK, "stack changed"
            assert (machine.reg_read(UC_ARM_REG_CPSR) >> 5) & 1 == thumb_return
            assert [machine.reg_read(r) for r in preserved] == register_values
            assert bytes(machine.mem_read(BITMAP, len(data))) == data, "bitmap changed"
            assert bytes(machine.mem_read(STACK - 32, 16)) == bytes([0xA5]) * 16
            assert bytes(machine.mem_read(STACK, 32)) == bytes([0xA5]) * 32
            results.append(machine.reg_read(UC_ARM_REG_R0))
        assert results[0] == results[1], (count, first, results, bitmap.hex())
        completed += 1

    padding = bytes([255]) * 512
    for pattern in (0, 255, 85, 170, 1, 128, 127, 254):
        for count in (0, 1, 2, 3, 4, 7, 8, 9, 16, 31, 32, 33, 64, 128, 512, 1024):
            for first in (
                0,
                1,
                2,
                7,
                8,
                9,
                24,
                25,
                31,
                32,
                127,
                255,
                511,
                960,
                991,
                992,
                999,
                1000,
                1016,
                1023,
                1024,
            ):
                compare(bytes([pattern]) * 128, padding, count, first)
    # Contiguous free runs crossing byte and word boundaries.
    for start in (0, 1, 7, 8, 9, 24, 25, 31, 32, 511, 960):
        for length in (1, 7, 8, 9, 31, 32, 33, 63, 64, 65, 128, 256):
            if start + length > 1024:
                continue
            bitmap = bytearray([255]) * 128
            for tile in range(start, start + length):
                bitmap[tile >> 3] &= ~(1 << (tile & 7))
            for first in (max(start - 1, 0), start, min(start + 1, 1024)):
                for count in (max(length - 1, 0), length, length + 1):
                    compare(bytes(bitmap), padding, count, first)
    rng = random.Random(seed)
    rng = random.Random(seed)
    for _ in range(random_cases):
        bitmap = bytes(
            rng.choices([0, 255, rng.randrange(256)], weights=[2, 5, 3], k=128)
        )
        tail = rng.randbytes(512)
        compare(bitmap, tail, rng.randrange(129), rng.randrange(1025))
    return completed


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=ROOT / "rotk.gba")
    parser.add_argument("--armips", default=shutil.which("armips") or "armips")
    parser.add_argument("--cases", type=int, default=20000)
    parser.add_argument("--seed", type=int, default=735)
    args = parser.parse_args()
    if args.cases < 0:
        parser.error("--cases must be nonnegative")
    rom = args.rom.read_bytes()
    expected = (ROOT / "baserom.sha1").read_text().split()[0]
    if hashlib.sha1(rom).hexdigest() != expected:
        raise SystemExit("original ROM checksum mismatch")
    started = time.monotonic()
    count = check(rom, assemble(args.armips), args.cases, args.seed)
    print(
        f"{count} native differential cases passed in {time.monotonic() - started:.2f}s"
    )


if __name__ == "__main__":
    main()
