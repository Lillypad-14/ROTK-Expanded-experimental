"""Execute the actual patched status helper, including its four-argument entry hook."""

from pathlib import Path
import struct
import subprocess
import tempfile

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC

ROOT = Path(__file__).resolve().parents[2]
BIN = Path.home() / "AppData/Local/rotk-tools/arm-gcc/bin"
FIELDS = {
    "players": "offsetof(PlayerGlobals, players)",
    "hero": "offsetof(Player, heroId)",
    "shield": "offsetof(Player, shieldBubble)",
    "stats": "offsetof(Player, stats)",
    "unit": "offsetof(Actor, unitType)",
    "player": "offsetof(Actor, as.combat.playerIndex)",
    "venom": "offsetof(Actor, as.combat.venomDuration)",
    "shieldHp": "offsetof(Actor, as.shieldGandalf.shieldHealth)",
    "source": "offsetof(IncomingResolution, source)",
    "victim": "offsetof(IncomingResolution, victim)",
    "resolutionPlayer": "offsetof(IncomingResolution, playerIndex)",
    "applied": "offsetof(IncomingResolution, appliedStatusElements)",
    "enemySize": "sizeof(EnemyTypeInfo)",
    "enemyFlags": "offsetof(EnemyTypeInfo, attackFlags)",
    "enemyPower": "offsetof(EnemyTypeInfo, poisonPower)",
    "resist": "STAT_POISON_RESIST_PERCENT",
    "armor": "STAT_MISSILE_ARMOR",
    "fireArmor": "STAT_FIRE_PROTECTION",
    "gandalf": "HERO_ID_GANDALF",
}
with tempfile.TemporaryDirectory(prefix="rotk-element-arm-") as directory:
    tmp = Path(directory)
    source = tmp / "layout.c"
    source.write_text(
        '#include <stddef.h>\n#include "patches/skill_expansion/skill_expansion_elite.c"\n'
        'const unsigned Layout[] __attribute__((section(".audit_layout"))) = {\n'
        + ",\n".join(FIELDS.values()) + "\n};\n"
    )
    subprocess.run([
        str(BIN / "arm-none-eabi-gcc.exe"), "-c", "-O2", "-std=gnu11", "-mthumb",
        "-mcpu=arm7tdmi", "-fno-short-enums",
        *(f"-I{ROOT / name}" for name in (
            "include", "carved/include", "build/include", "build/hack_include", "hacks")),
        str(source), "-o", str(tmp / "layout.o")], check=True)
    subprocess.run([str(BIN / "arm-none-eabi-objcopy.exe"), "-O", "binary", "-j",
                    ".audit_layout", str(tmp / "layout.o"), str(tmp / "layout.bin")], check=True)
    offsets = dict(zip(FIELDS, struct.unpack("<" + "I" * len(FIELDS),
                                             (tmp / "layout.bin").read_bytes())))

symbols = {}
for line in subprocess.check_output([str(BIN / "arm-none-eabi-nm.exe"), "-n",
                                      str(ROOT / "build/hack.elf")], text=True).splitlines():
    pieces = line.split()
    if len(pieces) == 3:
        symbols[pieces[2]] = int(pieces[0], 16)
for line in (ROOT / "config/functions.cfg").read_text().splitlines():
    fields = line.split()
    if len(fields) == 3 and fields[0].startswith("0x"):
        symbols.setdefault(fields[2], int(fields[0], 16))
rom = (ROOT / "build/rotk_hack.gba").read_bytes()
types = symbols["EnemyTypeInfos"] - 0x08000000
poison = next(i for i in range(83) if (
    struct.unpack_from("<H", rom, types + i * offsets["enemySize"] + offsets["enemyFlags"])[0] & 12
) == 12 and not (
    struct.unpack_from("<H", rom, types + i * offsets["enemySize"] + offsets["enemyFlags"])[0] & 32
))
fire = next(i for i in range(83) if (
    struct.unpack_from("<H", rom, types + i * offsets["enemySize"] + offsets["enemyFlags"])[0] & 20
) == 20 and not (
    struct.unpack_from("<H", rom, types + i * offsets["enemySize"] + offsets["enemyFlags"])[0] & 40
))

def check(raw: int, resistance: int, blocked: bool, enemy: int = poison) -> None:
    uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    for address, size in [(0x08000000, 0x02000000), (0x02000000, 0x40000),
                          (0x03000000, 0x8000), (0x04000000, 0x1000)]:
        uc.mem_map(address, size)
    uc.mem_write(0x08000000, rom)
    def write(address: int, value: int, size: int = 4) -> None:
        uc.mem_write(address, (value & ((1 << (8 * size)) - 1)).to_bytes(size, "little"))
    def read(address: int, size: int = 4) -> int:
        return int.from_bytes(uc.mem_read(address, size), "little")
    attacker, victim, shield, frame, context, sfx, stop = (
        0x02010000, 0x02011000, 0x02012000, 0x02013000, 0x02014000, 0x02015000, 0x0203c000)
    player = symbols["g_PlayerGlobals"] + offsets["players"]
    write(attacker + offsets["unit"], enemy, 1)
    write(victim + offsets["player"], 0, 1)
    write(victim + offsets["venom"], 90, 2)
    write(player + offsets["hero"], offsets["gandalf"], 1)
    write(player + offsets["shield"], shield)
    write(shield + offsets["shieldHp"], 50, 2)
    write(player + offsets["stats"] + offsets["resist"] * 2, resistance, 2)
    write(player + offsets["stats"] + offsets["armor"] * 2, 7, 2)
    write(player + offsets["stats"] + offsets["fireArmor"] * 2, 9, 2)
    write(frame + offsets["source"], context)
    write(frame + offsets["victim"], victim)
    write(frame + offsets["resolutionPlayer"], 0, 1)
    write(symbols["currentIncomingResolution"], frame)
    write(symbols["currentSource"], context)
    calls = []
    def hook(machine: Uc, address: int, size: int, data: object) -> None:
        if address == stop:
            machine.emu_stop()
        elif address == (symbols["math_percentOf"] & ~1):
            machine.reg_write(UC_ARM_REG_R0,
                              machine.reg_read(UC_ARM_REG_R0) * machine.reg_read(UC_ARM_REG_R1) // 100)
            machine.reg_write(UC_ARM_REG_PC, machine.reg_read(UC_ARM_REG_LR))
        elif address in {symbols["player_addHp"] & ~1, symbols["player_addSpirit"] & ~1}:
            calls.append(address)
            machine.reg_write(UC_ARM_REG_PC, machine.reg_read(UC_ARM_REG_LR))
    uc.hook_add(UC_HOOK_CODE, hook)
    for reg, value in [(UC_ARM_REG_R0, attacker), (UC_ARM_REG_R1, victim),
                       (UC_ARM_REG_R2, sfx), (UC_ARM_REG_R3, raw),
                       (UC_ARM_REG_SP, 0x03007e00), (UC_ARM_REG_LR, stop | 1)]:
        uc.reg_write(reg, value)
    uc.emu_start(symbols["combat_handleEnemyAttackOnPlayerStatusArmorSfx"] | 1, stop, count=100000)
    assert uc.reg_read(UC_ARM_REG_PC) == stop, hex(uc.reg_read(UC_ARM_REG_PC))
    assert uc.reg_read(UC_ARM_REG_SP) == 0x03007e00
    expected = 0 if blocked or resistance >= 100 or enemy == fire else 2
    assert read(frame + offsets["applied"], 1) == expected, (raw, resistance, expected)
    assert uc.reg_read(UC_ARM_REG_R0) == (16 if enemy == fire else 7)
    assert bool(calls) == (not blocked and enemy != fire)
    print(f"PASS: native ARM status hook raw={raw:#x}, resist={resistance}, applied={expected}")

check(20, 0, True)
check(80, 0, False)
check(80, 100, False)
check(0x10050, 0, False)
check(0xffff, 0, True)
check(80, 0, False, fire)
