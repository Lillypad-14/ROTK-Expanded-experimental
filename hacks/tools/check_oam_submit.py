"""Compare the sprite OAM writer patch with the original ROM on ARMv4T.

Run: uv run --with unicorn python hacks/tools/check_oam_submit.py
"""

import argparse
import hashlib
from pathlib import Path
import random
import shutil
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
CODE = 0x08036540
SIZE = 0x50
COUNT = 0x03004220
SHADOW_POINTER = 0x03004B40
SHADOW = 0x03004340
STOP = 0x03007F00
STACK = 0x03007E00
SOURCE = 0x02000000


def assemble(armips: str) -> bytes:
    with tempfile.TemporaryDirectory() as directory:
        folder = Path(directory)
        binary = folder / "submit.bin"
        source = folder / "submit.asm"
        patch = ROOT / "hacks/patches/fast_oam/fast_oam.asm"
        source.write_text(
            f'.gba\n.create "{binary.as_posix()}", {CODE:#x}\n'
            f".definelabel sprite_pushOam, {CODE:#x}\n"
            f".definelabel g_SpriteAllocCount, {COUNT:#x}\n"
            f".definelabel g_OamShadow, {SHADOW_POINTER:#x}\n"
            f'.include "{patch.as_posix()}"\n.close\n'
        )
        subprocess.run(
            [armips, str(source)], check=True, capture_output=True, text=True
        )
        code = binary.read_bytes()
    assert len(code) == SIZE, len(code)
    return code


def check(rom: bytes, optimized: bytes, random_cases: int, seed: int) -> int:
    from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM
    from unicorn.arm_const import (
        UC_CPU_ARM_TI925T,
        UC_ARM_REG_CPSR,
        UC_ARM_REG_LR,
        UC_ARM_REG_PC,
        UC_ARM_REG_SP,
        UC_ARM_REG_R0,
        UC_ARM_REG_R1,
        UC_ARM_REG_R4,
        UC_ARM_REG_R5,
        UC_ARM_REG_R6,
        UC_ARM_REG_R7,
        UC_ARM_REG_R8,
        UC_ARM_REG_R9,
        UC_ARM_REG_R10,
        UC_ARM_REG_R11,
    )

    machines = []
    for code in (rom[CODE - 0x08000000 : CODE - 0x08000000 + SIZE], optimized):
        machine = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        machine.ctl_set_cpu_model(UC_CPU_ARM_TI925T)
        machine.mem_map(0x08036000, 0x1000)
        machine.mem_map(0x03000000, 0x8000)
        machine.mem_map(SOURCE, 0x1000)
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
    registers = [0x12340000 + i for i in range(len(preserved))]
    rng = random.Random(seed)
    completed = 0

    def compare(index: int, count: int, source: int, shadow: int) -> None:
        nonlocal completed
        data = bytearray(rng.randbytes(0x2000))
        data[COUNT - 0x03004000] = count
        struct.pack_into("<I", data, SHADOW_POINTER - 0x03004000, shadow)
        entry = rng.randbytes(8)
        results = []
        thumb_return = completed & 1
        for machine in machines:
            machine.mem_write(0x03004000, bytes(data))
            machine.mem_write(SOURCE, entry)
            machine.mem_write(STACK - 32, bytes([0xA5]) * 64)
            machine.reg_write(UC_ARM_REG_CPSR, 0x1F)
            machine.reg_write(UC_ARM_REG_SP, STACK)
            machine.reg_write(UC_ARM_REG_LR, STOP | thumb_return)
            machine.reg_write(UC_ARM_REG_R0, index)
            machine.reg_write(UC_ARM_REG_R1, source)
            for register, value in zip(preserved, registers):
                machine.reg_write(register, value)
            machine.emu_start(CODE | 1, STOP, count=100)
            assert machine.reg_read(UC_ARM_REG_PC) == STOP
            assert machine.reg_read(UC_ARM_REG_SP) == STACK
            assert [machine.reg_read(r) for r in preserved] == registers
            assert bool(machine.reg_read(UC_ARM_REG_CPSR) & 32) == bool(thumb_return)
            assert machine.mem_read(STACK - 32, 20) == bytes([0xA5]) * 20
            assert machine.mem_read(STACK, 32) == bytes([0xA5]) * 32
            results.append(
                (
                    machine.reg_read(UC_ARM_REG_R0),
                    bytes(machine.mem_read(0x03004000, 0x2000)),
                    bytes(machine.mem_read(SOURCE, 8)),
                )
            )
        assert results[0] == results[1], (index, count, hex(source), hex(shadow))
        completed += 1

    for shadow in (SHADOW, 0x03004B80):
        for index in range(256):
            for count in (0, 1, 126, 127, 128, 129, 255):
                compare(index, count, SOURCE, shadow)
        for count in range(256):
            compare(rng.getrandbits(32), count, SOURCE, shadow)
        for index in (0, 1, 63, 127, 128, 255):
            for delta in (-4, 0, 4, 8, 1020, 1024, 1028):
                for count in (0, 127, 128, 255):
                    compare(index, count, shadow + index * 8 + delta, shadow)
    for _ in range(random_cases):
        compare(
            rng.getrandbits(32),
            rng.randrange(256),
            SOURCE,
            rng.choice((SHADOW, 0x03004B80)),
        )
    return completed


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=ROOT / "rotk.gba")
    parser.add_argument("--armips", default=shutil.which("armips") or "armips")
    parser.add_argument("--cases", type=int, default=10000)
    parser.add_argument("--seed", type=int, default=36540)
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
