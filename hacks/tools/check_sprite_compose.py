"""Differentially exercise two ROMs' sprite frame composition on ARMv4T."""

import argparse
import hashlib
from pathlib import Path
import random
import struct
import time

ROOT = Path(__file__).resolve().parents[2]
CODE = 0x08002CD8
SPAN = slice(0x2CD8, 0x304C)
STOP = 0x03007F00
STACK = 0x03007D00
ACTOR = 0x02001000
POSITION = 0x02002000
FRAMES = 0x02003000
BASE = 0x02004000


def check(baseline: bytes, optimized: bytes, cases: int, seed: int) -> int:
    from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM
    from unicorn.arm_const import (
        UC_CPU_ARM_TI925T,
        UC_ARM_REG_CPSR,
        UC_ARM_REG_R0,
        UC_ARM_REG_R1,
        UC_ARM_REG_R2,
        UC_ARM_REG_R3,
        UC_ARM_REG_R4,
        UC_ARM_REG_R5,
        UC_ARM_REG_R6,
        UC_ARM_REG_R7,
        UC_ARM_REG_R8,
        UC_ARM_REG_R9,
        UC_ARM_REG_R10,
        UC_ARM_REG_R11,
        UC_ARM_REG_SP,
        UC_ARM_REG_LR,
        UC_ARM_REG_PC,
    )

    assert len(baseline) == len(optimized) == 0x2000000
    assert baseline[0x2CD8:0x2CE8] != optimized[0x2CD8:0x2CE8], (
        "replacement not enabled"
    )
    machines = []
    for rom in (baseline, optimized):
        m = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        m.ctl_set_cpu_model(UC_CPU_ARM_TI925T)
        m.mem_map(0x08000000, 0x2000000)
        m.mem_write(0x08000000, rom)
        m.mem_map(0x02000000, 0x40000)
        m.mem_map(0x03000000, 0x8000)
        machines.append(m)
    saved = (
        UC_ARM_REG_R4,
        UC_ARM_REG_R5,
        UC_ARM_REG_R6,
        UC_ARM_REG_R7,
        UC_ARM_REG_R8,
        UC_ARM_REG_R9,
        UC_ARM_REG_R10,
        UC_ARM_REG_R11,
    )
    values = [0x12340000 + i for i in range(8)]
    rng = random.Random(seed)
    total = 0

    def compare(
        count: int,
        mode: int,
        flags: int,
        matrix: int,
        scale: int,
        x: int,
        y: int,
        allocation: int,
        world: int,
        shape: int,
        hotspots: int,
        boxes: int,
        frame_index: int,
    ) -> None:
        nonlocal total
        data = bytearray(rng.randbytes(0x5000))
        struct.pack_into("<II", data, ACTOR - 0x02000000 + 0x12C, scale, scale)
        struct.pack_into("<ii", data, POSITION - 0x02000000, x, y)
        header = FRAMES - 0x02000000
        # Three frame offsets exercise nonzero frame indices.
        struct.pack_into("<H", data, header + 6, 3)
        data[header + 10] = hotspots
        data[header + 11] = boxes
        for index in range(3):
            struct.pack_into("<H", data, header + 12 + index * 2, 6)
        frame = header + 18
        data[frame] = count | rng.choice((0, 32, 64, 96))
        pieces = frame + 8 + hotspots * 2 + boxes * 6
        for index in range(count):
            px = rng.randrange(-256, 256)
            py = rng.randrange(-256, 256)
            packed = (
                (px & 511)
                | ((py & 511) << 9)
                | (rng.randrange(4) << 18)
                | (shape << 20)
                | (rng.randrange(1024) << 22)
            )
            struct.pack_into("<I", data, pieces + index * 4, packed)
        word0 = rng.getrandbits(32) & ~((3 << 8) | (31 << 25))
        word0 |= mode << 8 | matrix << 25
        struct.pack_into("<II", data, BASE - 0x02000000, word0, rng.getrandbits(32))
        ram = bytearray(rng.randbytes(0x2000))
        struct.pack_into("<I", ram, 0x144, world)
        ram[0x220] = allocation
        struct.pack_into("<I", ram, 0xB40, rng.choice((0x03004340, 0x03004B80)))
        tile = rng.randrange(65536)
        results = []
        for m in machines:
            m.mem_write(0x02000000, bytes(data))
            m.mem_write(0x03004000, bytes(ram))
            m.mem_write(STACK - 256, bytes([0xA5]) * 512)
            m.mem_write(STACK, struct.pack("<III", tile, BASE, ACTOR))
            m.reg_write(UC_ARM_REG_CPSR, 0x1F)
            m.reg_write(UC_ARM_REG_SP, STACK)
            m.reg_write(UC_ARM_REG_LR, STOP | (total & 1))
            for register, value in zip(saved, values):
                m.reg_write(register, value)
            for register, value in zip(
                (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3),
                (FRAMES, frame_index, flags, POSITION),
            ):
                m.reg_write(register, value)
            m.emu_start(CODE | 1, STOP, count=100000)
            assert m.reg_read(UC_ARM_REG_PC) == STOP, (total, "instruction limit")
            assert m.reg_read(UC_ARM_REG_SP) == STACK
            assert [m.reg_read(r) for r in saved] == values
            assert bool(m.reg_read(UC_ARM_REG_CPSR) & 32) == bool(total & 1)
            assert m.mem_read(STACK - 256, 16) == bytes([0xA5]) * 16
            assert m.mem_read(STACK + 12, 244) == bytes([0xA5]) * 244
            results.append(
                (
                    bytes(m.mem_read(0x02000000, 0x5000)),
                    bytes(m.mem_read(0x03004000, 0x2000)),
                    bytes(m.mem_read(STACK, 12)),
                )
            )
        assert results[0] == results[1], (
            total,
            count,
            mode,
            flags,
            matrix,
            scale,
            x,
            y,
            allocation,
            world,
        )
        total += 1

    for mode in range(4):
        for flags in range(8):
            for matrix in (0, 8, 16, 24, 31):
                for count in (0, 1, 4, 31):
                    compare(count, mode, flags, matrix, 1024, 0, 0, 0, 0, 0, 0, 0, 0)
    for index in range(cases):
        compare(
            rng.randrange(32),
            rng.randrange(4),
            rng.randrange(256),
            rng.randrange(32),
            rng.choice((0, 1, 128, 256, 512, 1024, 2048, 4096)),
            rng.randrange(-400, 401),
            rng.randrange(-400, 401),
            rng.choice((0, 1, 126, 127, 128, 129, 255)),
            rng.choice((0, 1 << 16, 1 << 25, (1 << 25) | (1 << 16), 0xFFFFFFFF)),
            rng.randrange(3),
            rng.randrange(5),
            rng.randrange(5),
            rng.randrange(3),
        )
    return total


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--original", type=Path, default=ROOT / "rotk.gba")
    parser.add_argument("--write-baseline", type=Path)
    parser.add_argument("--optimized", type=Path, required=True)
    parser.add_argument("--cases", type=int, default=5000)
    parser.add_argument("--seed", type=int, default=2484)
    args = parser.parse_args()
    if args.cases < 0:
        parser.error("--cases must be nonnegative")
    original = args.original.read_bytes()
    expected = (ROOT / "baserom.sha1").read_text().split()[0]
    if hashlib.sha1(original).hexdigest() != expected:
        raise SystemExit("original ROM checksum mismatch")
    optimized = args.optimized.read_bytes()
    if args.baseline:
        baseline = args.baseline.read_bytes()
    else:
        restored = bytearray(optimized)
        restored[SPAN] = original[SPAN]
        baseline = bytes(restored)
    if baseline[SPAN] != original[SPAN]:
        raise SystemExit("baseline frame routine does not match the original ROM")
    if args.write_baseline:
        args.write_baseline.write_bytes(baseline)
    started = time.monotonic()
    count = check(baseline, optimized, args.cases, args.seed)
    print(
        f"{count} sprite composition differential cases passed in {time.monotonic() - started:.2f}s"
    )
    print("Baseline SHA256:", hashlib.sha256(baseline).hexdigest())
    print("Optimized SHA256:", hashlib.sha256(optimized).hexdigest())


if __name__ == "__main__":
    main()
