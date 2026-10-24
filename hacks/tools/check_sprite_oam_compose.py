"""Compare native and replacement actor sprite composition on ARMv4T."""

import argparse
import hashlib
from pathlib import Path
import random
import struct
import time

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_CODE
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
    UC_ARM_REG_R12,
    UC_ARM_REG_SP,
    UC_ARM_REG_LR,
    UC_ARM_REG_PC,
)

ROOT = Path(__file__).resolve().parents[2]
CODE = 0x08002484
CAMERA = 0x0803F214
STOP = 0x03007F00
STACK = 0x03007D00
ACTOR = 0x02001000
OWNER = 0x02001400
BANK = 0x02003000
ANIMATIONS = 0x02006000
FRAMES = 0x02008000
CELLS = 0x02007000


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument(
        "--candidate", type=Path, help="compile a C draft with the period compiler"
    )
    source.add_argument("--optimized", type=Path, help="test a built ROM")
    parser.add_argument("--original", type=Path, default=ROOT / "rotk.gba")
    parser.add_argument(
        "--cases",
        type=int,
        default=5000,
        help="random cases in addition to fixed coverage",
    )
    parser.add_argument("--seed", type=int, default=2484)
    args = parser.parse_args()
    if args.cases < 0:
        parser.error("--cases must be nonnegative")
    original = args.original.read_bytes()
    expected = (ROOT / "baserom.sha1").read_text().split()[0]
    if hashlib.sha1(original).hexdigest() != expected:
        raise SystemExit("original ROM checksum mismatch")
    if args.candidate:
        from rotkit.compile import compile_file

        base, candidate = compile_file(str(args.candidate), werror=False)
        if base != CODE or len(candidate) > 0x2CD8 - 0x2484:
            raise SystemExit("candidate must fit the native main-composer span")
        replacement = bytearray(original)
        replacement[base - 0x08000000 : base - 0x08000000 + len(candidate)] = candidate
    else:
        replacement = args.optimized.read_bytes()
    if len(replacement) not in (0x1000000, 0x2000000):
        raise SystemExit("replacement ROM must be 16 or 32 MiB")
    rng = random.Random(args.seed)
    camera_position: tuple[int, int] = (0, 0)
    camera_calls: list[int] = []

    def camera(m: Uc, _address: int, _size: int, _user_data: object) -> None:
        camera_calls.append(m.reg_read(UC_ARM_REG_R1))
        m.mem_write(m.reg_read(UC_ARM_REG_R0), struct.pack("<ii", *camera_position))
        for index, register in enumerate(
            (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_R12)
        ):
            m.reg_write(register, 0xDEAD0000 + index)
        m.reg_write(UC_ARM_REG_PC, m.reg_read(UC_ARM_REG_LR))

    machines = []
    for rom in (original, bytes(replacement)):
        m = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        m.ctl_set_cpu_model(UC_CPU_ARM_TI925T)
        m.mem_map(0x08000000, 0x2000000)
        m.mem_write(0x08000000, rom)
        m.mem_map(0x02000000, 0x40000)
        m.mem_map(0x03000000, 0x8000)
        m.hook_add(UC_HOOK_CODE, camera, begin=CAMERA, end=CAMERA)
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
    saved_values = [0x12340000 + i for i in range(8)]
    started = time.monotonic()
    fixed = [
        (sprite_flags, flags, strip, affine, missing, current)
        for sprite_flags in (0, 1, 0x20, 0x40, 0x60, 0x80, 0xFF)
        for flags in (0, 0x40000000, 0x40000100)
        for strip in (0, 1)
        for affine in range(4)
        for missing in (False, True)
        for current in range(3)
    ]
    bank_data = bytearray(384 * 7 * 4)
    for part in range(-128, 256):
        for pose in range(7):
            struct.pack_into(
                "<I",
                bank_data,
                ((part + 128) * 7 + pose) * 4,
                ANIMATIONS + ((part % 3) * 7 + pose) * 80,
            )
    total = len(fixed) + args.cases
    for case in range(total):
        data = bytearray(rng.randbytes(0xC000))
        data[BANK - 0x02000000 - 128 * 28 : BANK - 0x02000000 + 256 * 28] = bank_data
        ram = bytearray(rng.randbytes(0x8000))

        def put(address: int, fmt: str, *values: int) -> None:
            struct.pack_into(fmt, data, address - 0x02000000, *values)

        flags = rng.getrandbits(32)
        flags = (flags & ~0x40000100) | rng.choice((0, 0x40000000, 0x40000100))
        sprite_flags = rng.randrange(256)
        strip = rng.randrange(4) == 0
        current = rng.randrange(3)
        affine = rng.randrange(4)
        missing = rng.randrange(4) == 0
        if case < len(fixed):
            sprite_flags, flags, strip, affine, missing, current = fixed[case]
        put(ACTOR + 0xE8, "<I", flags)
        put(
            ACTOR + 0xFC,
            "<BBBB",
            sprite_flags,
            rng.randrange(4),
            strip,
            rng.randrange(2),
        )
        put(ACTOR + 0xF2, "<H", (0xFFFF if missing else rng.choice((0, 7, 1000))))
        put(ACTOR + 0x22, "<BB", rng.randrange(3), current)
        put(ACTOR + 0x24, "<I", rng.randrange(5) if strip else ANIMATIONS)
        put(OWNER + 0x24, "<I", ANIMATIONS + 16)
        put(OWNER + 0x23, "<B", rng.randrange(3))
        put(
            ACTOR + 0xCC,
            "<ii",
            rng.randrange(-500, 501) * 65536,
            rng.randrange(-500, 501) * 65536,
        )
        put(
            ACTOR + 0x12C,
            "<II",
            rng.choice((256, 512, 1024, 2048)),
            rng.choice((256, 512, 1024, 2048)),
        )
        word = rng.getrandbits(32) & ~((3 << 14) | (3 << 8))
        word |= rng.randrange(3) << 14 | affine << 8
        put(ACTOR + 0xDC, "<II", word, rng.getrandbits(32))
        put(ACTOR + 0x104, "<I", BANK)
        put(ACTOR + 0x108, "<HHHH", *(rng.randrange(1024) for _ in range(4)))
        put(
            ACTOR + 0x118,
            "<BBBB",
            *(rng.choice((0, 1, 2, 127, 128, 254, 255)) for _ in range(4)),
        )
        put(ACTOR + 0x11C, "<BBBB", *(rng.randrange(16) for _ in range(4)))
        put(ACTOR + 0x120, "<BBBB", *(rng.randrange(8) for _ in range(4)))
        put(ACTOR + 0x124, "<BBBB", *(rng.randrange(16) for _ in range(4)))
        put(ACTOR + 0x128, "<BB", rng.randrange(7), rng.randrange(5))
        for part in range(3):
            for pose in range(7):
                for facing in range(5):
                    put(
                        ANIMATIONS + (part * 7 + pose) * 80 + facing * 16 + 4,
                        "<I",
                        FRAMES + part * 0x400,
                    )
        for group in range(3):
            frames = FRAMES + group * 0x400
            put(frames + 6, "<H", 3)
            hotspots, boxes = rng.randrange(5), rng.randrange(5)
            put(frames + 10, "<BB", hotspots, boxes)
            for frame_index in range(3):
                offset = 6 + frame_index * 160
                put(frames + 12 + frame_index * 2, "<H", offset)
                frame = frames + 12 + offset
                count = rng.choice((0, 1, 2, 8, 16))
                put(frame, "<B", count)
                put(frame + 4, "<H", rng.randrange(1024))
                pieces = frame + 8 + boxes * 6 + hotspots * 2
                for index in range(count):
                    bits = (
                        rng.randrange(512)
                        | rng.randrange(512) << 9
                        | rng.randrange(4) << 18
                        | rng.randrange(3) << 20
                        | rng.randrange(1024) << 22
                    )
                    put(pieces + index * 4, "<I", bits)
        for cell in range(4):
            put(CELLS + cell * 44, "<I", OWNER)
            put(CELLS + cell * 44 + 6, "<H", rng.randrange(1024))
        struct.pack_into("<I", ram, 0x19EC, CELLS)
        struct.pack_into(
            "<I", ram, 0x4144, rng.choice((0, 1 << 16, 1 << 25, 0xFFFFFFFF))
        )
        ram[0x4220] = rng.choice((0, 1, 120, 127, 128, 255))
        struct.pack_into("<I", ram, 0x4B40, rng.choice((0x03004340, 0x03004B80)))
        ram[0x3F59] = rng.randrange(2)
        camera_position = (rng.randrange(-300, 301), rng.randrange(-300, 301))
        results = []
        for m in machines:
            camera_calls.clear()
            m.mem_write(0x02000000, bytes(data))
            m.mem_write(0x03000000, bytes(ram))
            m.mem_write(STACK - 512, bytes([0xA5]) * 1024)
            m.reg_write(UC_ARM_REG_CPSR, 0x1F)
            m.reg_write(UC_ARM_REG_R0, ACTOR)
            m.reg_write(UC_ARM_REG_SP, STACK)
            m.reg_write(UC_ARM_REG_LR, STOP | (case & 1))
            for register, value in zip(saved, saved_values):
                m.reg_write(register, value)
            m.emu_start(CODE | 1, STOP, count=200000)
            assert m.reg_read(UC_ARM_REG_PC) == STOP, (case, "instruction limit")
            assert m.reg_read(UC_ARM_REG_SP) == STACK, case
            assert [m.reg_read(r) for r in saved] == saved_values, case
            assert bool(m.reg_read(UC_ARM_REG_CPSR) & 32) == bool(case & 1), case
            assert m.mem_read(STACK, 512) == bytes([0xA5]) * 512, case
            assert m.mem_read(STACK - 512, 16) == bytes([0xA5]) * 16, case
            results.append(
                (
                    m.reg_read(UC_ARM_REG_R0),
                    bytes(m.mem_read(0x02000000, 0xC000)),
                    bytes(m.mem_read(0x03000000, STACK - 512 - 0x03000000)),
                    tuple(camera_calls),
                )
            )
        if results[0] != results[1]:
            raise AssertionError(
                (
                    case,
                    hex(flags),
                    hex(sprite_flags),
                    strip,
                    [r[0] for r in results],
                    [r[3] for r in results],
                )
            )
    print(
        f"{total} actor composition differential cases passed in {time.monotonic() - started:.2f}s"
    )
    print("Baseline SHA256:", hashlib.sha256(original).hexdigest())
    print("Replacement SHA256:", hashlib.sha256(replacement).hexdigest())


if __name__ == "__main__":
    main()
