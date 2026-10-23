"""Execute the built ARM skill and equipment code with deterministic world/FX stubs."""

from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import traceback

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3
from unicorn.arm_const import UC_ARM_REG_PC, UC_ARM_REG_LR, UC_ARM_REG_SP

ROOT = Path(__file__).resolve().parents[2]
BIN = Path.home() / "AppData/Local/rotk-tools/arm-gcc/bin"
LAYOUT = {"Player.size": "sizeof(Player)", "Actor.size": "sizeof(Actor)",
          "SkillExpansionPlayer.size": "sizeof(SkillExpansionPlayer)",
          "RingwraithState.size": "sizeof(RingwraithState)",
          "DreadMark.size": "sizeof(DreadMark)", "EnemyTypeInfo.size": "sizeof(EnemyTypeInfo)",
          "SaveSlot.size": "sizeof(SaveSlot)",
          "ModOptionEntry.size": "sizeof(struct ModOptionEntry)",
          "ActiveSkill.size": "sizeof(ActiveSkill)", "ItemStatRecord.size": "sizeof(ItemStatRecord)"}
for typename, fields in {
    "Player": "heroId ownerActor stats currentHp currentSpirit combatFlags inventory activeSkills passiveSkillIds quickSelectActiveSkills level castingActiveSkill activeSkillLevels activeSkillLevelsPurchased passiveSkillLevelsPurchased passiveSkillLevelsApplied selectedActiveSkill activeSkillStateFlags activeSkillCycleEnabled",
    "Actor": "coords renderCoords flags type variant unitType currentHp behaviorState actionState actionStateTimer parentActor tickCallback as triggerBoxes triggerCallbacks movementCollisionBox spriteFlags velocity",
    "SkillExpansionPlayer": "activeSkillLevels passiveSkillLevelsApplied passiveSkillLevelsPurchased activeSkillLevelsPurchased activeSkillCycleEnabled",
    "RingwraithState": "breathFrames unseenFrames maliceFrames lastSpirit allowGain unseenStrike meleeHits valeFrames drainPercent drainRemainder sacrificeFrames willFrames mordorFrames mordorCenter mordorLevel stepStrike maskChain bladeGlowFrames",
    "ActiveSkill": "callback spiritCost",
    "GhostState": "target owner lifetime damage",
    "DreadMark": "actor dread",
    "EnemyTypeInfo": "attackFlags",
    "SceneState": "args id",
    "SkillExpansionRuntime": "owner kit callbacks paidSpirit feedbackFrames pendingRow pending initialized heroId feedback lastResult",
    "SkillExpansionBurst": "attacker center exclude radius damage stunFrames kind abilityRow",
    "SkillExpansionHeroKit": "onHpChange incomingDamage onKill onHit",
    "HackLibDamageSource": "origin kind abilityRow depth",
    "PlayerHud": "text textBuffer flags",
    "struct ModOptionEntry": "set get",
}.items():
    for field in fields.split():
        LAYOUT[f"{typename}.{field}"] = f"offsetof({typename}, {field})"


def layout():
    enabled = set()
    for line in (ROOT / "hacks/patches.cfg").read_text().splitlines():
        enabled.update(line.split("#")[0].split())
    flags = [f"-DHACK_{path.name}={int(path.name in enabled)}"
             for path in (ROOT / "hacks/patches").iterdir() if path.is_dir()]
    with tempfile.TemporaryDirectory(prefix="rotk-layout-") as directory:
        temporary = Path(directory)
        source, obj, binary = (temporary / name for name in ("layout.c", "layout.o", "layout.bin"))
        source.write_text('#include <stddef.h>\n#include "scene.h"\n#include "save.h"\n#include "patches/debug_cheats/debug_lab.h"\n#include "patches/skill_expansion/skill_expansion_abilities.h"\n#include "patches/ringwraith/ringwraith_kit.c"\n'
                          'const unsigned AuditLayout[] __attribute__((section(".audit_layout"))) = {\n'
                          + ",\n".join(LAYOUT.values()) + "\n};\n")
        subprocess.run([str(BIN / "arm-none-eabi-gcc.exe"), "-c", "-O2", "-std=gnu11",
                        "-mcpu=arm7tdmi", "-mthumb", "-fno-short-enums", *flags,
                        *[f"-I{ROOT / path}" for path in ("include", "carved/include", "build/include", "build/hack_include", "hacks")],
                        str(source), "-o", str(obj)], check=True, capture_output=True)
        subprocess.run([str(BIN / "arm-none-eabi-objcopy.exe"), "-O", "binary", "-j", ".audit_layout",
                        str(obj), str(binary)], check=True)
        return dict(zip(LAYOUT, struct.unpack(f"<{len(LAYOUT)}I", binary.read_bytes())))


OFF = layout()
symbols = {}
for line in (ROOT / "build/game_symbols.inc").read_text().splitlines():
    match = re.match(r"(\w+)\s+equ\s+(0x[0-9a-fA-F]+)", line)
    if match:
        symbols[match[1]] = int(match[2], 16)
nm = subprocess.check_output([str(BIN / "arm-none-eabi-nm.exe"), "-S", "-n", str(ROOT / "build/hack.elf")], text=True)
for line in nm.splitlines():
    parts = line.split()
    if len(parts) >= 3:
        symbols[parts[-1]] = int(parts[0], 16)
        if parts[-1] == "states" and len(parts) == 4 and int(parts[1], 16) == 2 * OFF["RingwraithState.size"]:
            symbols["wraithStates"] = int(parts[0], 16)


class Game:
    def __init__(self):
        self.cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
        for address, size in ((0x08000000, 0x02000000), (0x02000000, 0x40000), (0x03000000, 0x8000),
                              (0x04000000, 0x1000), (0x05000000, 0x1000), (0x06000000, 0x20000), (0x0E000000, 0x10000)):
            self.cpu.mem_map(address, size)
        self.cpu.mem_write(0x08000000, (ROOT / "build/rotk_hack.gba").read_bytes())
        self.hooks = {}
        self.enemies = []
        self.summons = []
        self.hit = True
        self.actor = 0x02010000
        self.player = symbols["g_PlayerGlobals"]
        self.sx = symbols["skillExpansion_players"]
        self.state = symbols["wraithStates"]
        self.silent = set()
        for name in ("hackLib_fxBurst", "hackLib_fxTimed", "hackLib_fxAt", "hackLib_fxSprite",
                     "hackLib_fxEnemyAnim", "hackLib_fxTintSlot", "hackLib_fxHold", "hackLib_fxRemove",
                     "sfx_play", "fx_attachStatusEffect"):
            self.stub(name, lambda: 0)
        self.stub("rng_rollRange", lambda: self.arg(0))
        self.stub("terrain_getWorldSize", lambda: self.cpu.mem_write(self.arg(0), struct.pack("<ii", 1024, 1024)))
        self.write(symbols["g_TerrainCollisionBlocks"], 0x02026000)
        self.write(symbols["g_TerrainCollisionCells"], 0x02028000)
        self.stub("findNextEnemy", self.next_enemy, address=0x080013a8)
        self.stub("actor_iterateInRadius", self.in_radius)
        self.stub("spawnNpc", self.spawn, address=0x08035864)
        self.stub("skillExpansion_original_combat_applyPveRangedHit", self.ranged_hit)
        self.stub("skillExpansion_original_player_addHp", lambda: self.add_resource("currentHp", 22))
        self.stub("ringwraith_original_player_addSpirit", lambda: self.add_resource("currentSpirit", 23))
        self.cpu.hook_add(UC_HOOK_CODE, self.dispatch)
        self.set_player("heroId", 8, 1)
        self.set_player("ownerActor", self.actor)
        self.set_player("currentHp", 100, 2)
        self.set_player("currentSpirit", 500, 2)
        self.set_player("level", 50, 1)
        for stat in range(90):
            self.write(self.player + OFF["Player.stats"] + stat * 2, 0, 2)
        self.stat(22, 500)
        self.stat(23, 500)
        for slot in range(16):
            self.equip(slot, 0xFFFFFFFF)
        self.write(self.actor + OFF["Actor.as"] + 18, 0, 1)  # combat.playerIndex
        self.write(self.actor + OFF["Actor.flags"], (1 << 2) | (1 << 12))
        self.write(self.actor + OFF["Actor.currentHp"], 100, 2)
        self.call("skillExpansion_initHeroLoadout", 8, 0)
        self.write(self.state + OFF["RingwraithState.lastSpirit"], 500, 2)
        for row in range(12):
            self.active(row, 1)

    def dispatch(self, cpu, address, size, data):
        callback = self.hooks.get(address)
        if callback:
            result = callback()
            cpu.reg_write(UC_ARM_REG_R0, (result or 0) & 0xFFFFFFFF)
            cpu.reg_write(UC_ARM_REG_PC, cpu.reg_read(UC_ARM_REG_LR))

    def stub(self, name, callback, address=None):
        self.hooks[(address if address is not None else symbols[name]) & ~1] = callback

    def arg(self, index):
        return self.cpu.reg_read((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3)[index])

    def call(self, name, *args):
        address = symbols[name] if isinstance(name, str) else name
        self.cpu.reg_write(UC_ARM_REG_SP, 0x0203D000)
        self.cpu.reg_write(UC_ARM_REG_LR, 0x0203E001)
        for register, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3), args):
            self.cpu.reg_write(register, value & 0xFFFFFFFF)
        for index, value in enumerate(args[4:]):
            self.write(0x0203D000 + index * 4, value)
        try:
            self.cpu.emu_start(address | 1, 0x0203E000, count=300000)
        except Exception as error:
            raise RuntimeError(f"{name}: PC={self.cpu.reg_read(UC_ARM_REG_PC):#x}: {error}") from error
        assert self.cpu.reg_read(UC_ARM_REG_PC) == 0x0203E000, f"{name}: instruction limit"
        return self.cpu.reg_read(UC_ARM_REG_R0)

    def read(self, address, size=4, signed=False):
        return int.from_bytes(self.cpu.mem_read(address, size), "little", signed=signed)

    def write(self, address, value, size=4):
        self.cpu.mem_write(address, (value & ((1 << (8 * size)) - 1)).to_bytes(size, "little"))

    def set_player(self, field, value, size=4):
        self.write(self.player + OFF[f"Player.{field}"], value, size)

    def get_player(self, field, size=2):
        return self.read(self.player + OFF[f"Player.{field}"], size)

    def stat(self, stat, value):
        self.write(self.player + OFF["Player.stats"] + stat * 2, value, 2)

    def state_value(self, field):
        return self.read(self.state + OFF[f"RingwraithState.{field}"], 2)

    def set_state(self, field, value):
        self.write(self.state + OFF[f"RingwraithState.{field}"], value, 2)

    def put_gear(self, slot, kind, base, prefix=255, suffix=255):
        self.equip(slot, item_word(kind, base, prefix, suffix))
        self.call("wraithGear_refresh", 0)

    def dread(self, enemy, amount):
        self.call("addDread", 0, enemy, amount)

    def get_dread(self, enemy):
        for i in range(24):
            mark = symbols["marks"] + i * OFF["DreadMark.size"]
            if self.read(mark + OFF["DreadMark.actor"]) == enemy:
                return self.read(mark + OFF["DreadMark.dread"], 1)
        return 0

    def fill_thralls(self):
        self.set_state("willFrames", 600)
        for _ in range(12):
            foe = self.enemy(x=16, hp=1)
            self.call("skillExpansion_applyPveRangedHit", self.actor, foe, 10)
        return len(self.summons)

    def equip(self, slot, item):
        self.write(self.player + OFF["Player.inventory"] + slot * 4, item)

    def active(self, row, level):
        if row < 6:
            address = self.player + OFF["Player.activeSkillLevels"] + row
        else:
            address = self.sx + OFF["SkillExpansionPlayer.activeSkillLevels"] + row - 6
        self.write(address, level, 1)

    def passive(self, slot, level):
        if slot < 9:
            address = self.player + OFF["Player.passiveSkillLevelsApplied"] + slot
        else:
            address = self.sx + OFF["SkillExpansionPlayer.passiveSkillLevelsApplied"] + slot - 9
        self.write(address, level, 1)

    def enemy(self, x=0, y=0, hp=100):
        actor = 0x02011000 + len(self.enemies) * 0x200
        self.enemies.append(actor)
        self.write(actor + OFF["Actor.coords"], x * 65536)
        self.write(actor + OFF["Actor.coords"] + 4, y * 65536)
        self.write(actor + OFF["Actor.flags"], (1 << 2) | (1 << 12))
        self.write(actor + OFF["Actor.type"], 1, 2)
        self.write(actor + OFF["Actor.unitType"], 0, 1)
        self.write(actor + OFF["Actor.currentHp"], hp, 2)
        return actor

    def next_enemy(self):
        cursor = self.arg(1)
        index = self.enemies.index(cursor) + 1 if cursor in self.enemies else 0
        return self.enemies[index] if index < len(self.enemies) else 0

    def in_radius(self):
        center, cursor, radius = self.arg(0), self.arg(1), self.arg(2)
        index = self.enemies.index(cursor) + 1 if cursor in self.enemies else 0
        for enemy in self.enemies[index:]:
            dx = self.read(enemy + OFF["Actor.coords"], signed=True) - self.read(center + OFF["Actor.coords"], signed=True)
            dy = self.read(enemy + OFF["Actor.coords"] + 4, signed=True) - self.read(center + OFF["Actor.coords"] + 4, signed=True)
            if dx * dx + dy * dy <= radius * radius:
                return enemy
        return 0

    def spawn(self):
        actor = 0x02020000 + len(self.summons) * 0x200
        self.summons.append(actor)
        self.cpu.mem_write(actor + OFF["Actor.coords"], bytes(self.cpu.mem_read(self.arg(1), 8)))
        self.write(actor + OFF["Actor.parentActor"], self.arg(2))
        self.write(actor + OFF["Actor.as"] + OFF["GhostState.owner"], self.arg(2))
        self.write(actor + OFF["Actor.as"] + OFF["GhostState.target"], self.arg(3))
        self.write(actor + OFF["Actor.as"] + OFF["GhostState.lifetime"], self.read(self.cpu.reg_read(UC_ARM_REG_SP)), 2)
        self.write(actor + OFF["Actor.type"], 20, 2)
        self.write(actor + OFF["Actor.variant"], 26, 1)
        self.write(actor + OFF["Actor.behaviorState"], 32, 1)
        actor_list = symbols["g_ActorListMain"]
        self.write(actor, self.read(actor_list))
        self.write(actor_list, actor)
        return actor

    def ranged_hit(self):
        victim = self.arg(1)
        if self.hit:
            hp = self.read(victim + OFF["Actor.currentHp"], 2, signed=True)
            self.write(victim + OFF["Actor.currentHp"], max(0, hp - self.arg(2)), 2)
        return int(self.hit)

    def add_resource(self, field, maximum):
        delta = self.arg(1) & 0xFFFF
        if delta & 0x8000:
            delta -= 65536
        limit = self.read(self.player + OFF["Player.stats"] + maximum * 2, 2)
        self.set_player(field, max(0, min(limit, self.get_player(field) + delta)), 2)
        return 0

    def cast(self, row):
        self.set_player("castingActiveSkill", row, 1)
        table = self.get_player("activeSkills", 4)
        callback = self.read(table + row * OFF["ActiveSkill.size"] + OFF["ActiveSkill.callback"])
        assert callback
        return self.call(callback, self.actor)

    def tick(self):
        self.call("ringwraith_kitTick", 0, self.actor)


def item_word(kind, base, prefix=255, suffix=255):
    return kind | 0xF0 | base << 8 | prefix << 16 | suffix << 24


def check_feedback_and_large_heals():
    game = Game()
    game.write(symbols["g_SceneCurrent"] + OFF["SceneState.id"], 1, 4)
    message = 0x02030000
    game.cpu.mem_write(message, b"No safe landing\0")
    game.call("skillExpansion_feedback", 0, message)
    game.call("skillExpansion_prepareHud", 0)
    hud = symbols["g_PlayerHuds"]
    assert game.read(hud + OFF["PlayerHud.text"]) == message
    assert game.read(hud + OFF["PlayerHud.flags"]) & 3 == 3
    game.write(hud + OFF["PlayerHud.text"], message + 64)
    game.write(hud + OFF["PlayerHud.flags"], 1)
    game.call("skillExpansion_prepareHud", 0)
    assert game.read(hud + OFF["PlayerHud.text"]) == message + 64, "feedback stole pickup text"
    game = Game()
    foe = game.enemy(hp=1000)
    game.stat(22, 32767)
    game.set_player("currentHp", 1, 2)
    game.write(game.state + OFF["RingwraithState.drainPercent"], 255, 1)
    game.call("ringwraith_kitOnHit", 0, game.actor, foe, 0, 1, 32767)
    assert game.get_player("currentHp") == 32767, "large heal wrapped through native signed delta"


def check_ability_helpers():
    game = Game()
    far, near = game.enemy(x=40, hp=100), game.enemy(x=16, hp=100)
    assert game.call("skillExpansion_findTarget", game.actor, 64, 0, 0) == near
    burst = 0x02030000
    for field, value, size in (("attacker", game.actor, 4), ("center", game.actor, 4),
                               ("exclude", near, 4), ("radius", 64, 2), ("damage", 10, 2),
                               ("kind", 3, 1), ("abilityRow", 3, 1)):
        game.write(burst + OFF[f"SkillExpansionBurst.{field}"], value, size)
    assert game.call("skillExpansion_burstDamage", burst) == 1
    assert game.read(far + OFF["Actor.currentHp"], 2) == 90
    assert game.read(near + OFF["Actor.currentHp"], 2) == 100
    ghost = game.call("skillExpansion_summonGhost", game.actor, game.actor + OFF["Actor.coords"],
                      near, 600, 1000, 2, 1)
    assert ghost and game.read(ghost + OFF["Actor.as"] + OFF["GhostState.damage"], 1) == 255
    assert game.call("skillExpansion_summonGhost", game.actor, game.actor + OFF["Actor.coords"], near, 600, 20, 2, 1)
    assert not game.call("skillExpansion_summonGhost", game.actor, game.actor + OFF["Actor.coords"], near, 600, 20, 2, 1)
    assert game.call("skillExpansion_consumeGhost", ghost, game.actor)
    assert not game.call("skillExpansion_consumeGhost", ghost, game.actor)
    game = Game()
    foe = game.enemy(hp=1000)
    allocated = []
    def factory():
        actor = 0x02018000 + len(allocated) * 0x200
        allocated.append(actor)
        game.write(actor + OFF["Actor.parentActor"], game.arg(0))
        game.write(actor, game.read(symbols["g_ActorListMain"]))
        game.write(symbols["g_ActorListMain"], actor)
        return actor
    game.stub("actor_projectile_create", factory)
    assert not game.call("skillExpansion_launchProjectile", game.actor, 32, 25, 4)
    shots = [game.call("skillExpansion_launchProjectile", game.actor, 8, 25, 4) for _ in range(8)]
    assert all(shots)
    assert not game.call("skillExpansion_launchProjectile", game.actor, 8, 25, 4)
    callback = game.read(shots[0] + OFF["Actor.triggerCallbacks"])
    game.call(callback, shots[0], foe)
    assert game.read(foe + OFF["Actor.currentHp"], 2) == 975
    game.call(callback, shots[0], foe)
    assert game.read(foe + OFF["Actor.currentHp"], 2) == 975
    assert game.call("skillExpansion_launchProjectile", game.actor, 8, 25, 4)


def check_shared_combat_hooks():
    game = Game()
    game.write(symbols["RingwraithHeroKit"] + OFF["SkillExpansionHeroKit.onHpChange"], 0)
    game.write(symbols["RingwraithHeroKit"] + OFF["SkillExpansionHeroKit.incomingDamage"], 0x0203B001)
    game.stub("incomingTest", lambda: game.arg(2) // 2, address=0x0203B000)
    game.call("player_addHp", 0, -20, 0)
    assert game.get_player("currentHp") == 90
    kills = []
    game.write(symbols["RingwraithHeroKit"] + OFF["SkillExpansionHeroKit.onKill"], 0x0203B101)
    game.stub("killTest", lambda: kills.append(game.arg(2)), address=0x0203B100)
    foe = game.enemy(hp=1)
    game.call("skillExpansion_applyPveRangedHit", game.actor, foe, 20)
    assert kills == [foe]


def check_cast_refunds():
    def ready(game):
        game.set_player("currentSpirit", 100, 2)
        game.write(game.state + OFF["RingwraithState.lastSpirit"], 100, 2)
    for free in (False, True):
        game = Game()
        foe = game.enemy(x=16, hp=1000)
        ready(game)
        game.write(symbols["g_GameFlags"], 4 if free else 0)
        game.call("skillExpansion_cast", 0, 0)
        assert game.get_player("currentSpirit") == (100 if free else 80)
        table = game.get_player("activeSkills", 4)
        game.write(table + OFF["ActiveSkill.spiritCost"], 99, 2)
        game.write(foe + OFF["Actor.currentHp"], 0, 2)
        game.call("skillExpansion_completeCast", game.actor)
        assert game.get_player("currentSpirit") == 100, "refund did not use actual paid cost"
        game.call("skillExpansion_completeCast", game.actor)
        assert game.get_player("currentSpirit") == 100, "double refund"
    game = Game()
    game.enemy(x=16, hp=1000)
    ready(game)
    game.call("skillExpansion_cast", 0, 0)
    game.set_player("activeSkillStateFlags", 0, 1)
    game.write(game.actor + OFF["Actor.actionState"], 0, 1)
    game.call("skillExpansion_runtimeTick", 0, game.actor)
    assert game.get_player("currentSpirit") == 100, "interrupted cast lost Spirit"
    game = Game()
    foe = game.enemy(x=16, hp=1000)
    ready(game)
    game.call("skillExpansion_cast", 0, 0)
    game.call("skillExpansion_completeCast", game.actor)
    assert game.get_player("currentSpirit") == 83, "successful cast should retain cost and grant 3 terror Spirit"
    assert game.read(foe + OFF["Actor.currentHp"], 2) < 1000
    game = Game()
    ready(game)
    cost = game.read(game.get_player("activeSkills", 4) + 5 * OFF["ActiveSkill.size"] + OFF["ActiveSkill.spiritCost"], 2)
    game.call("skillExpansion_cast", 0, 5)
    assert game.get_player("currentSpirit") == 100 - cost
    assert game.read(game.state + OFF["RingwraithState.breathFrames"], 2) > 0


def check_safe_landing_and_step():
    game = Game()
    preferred, output = 0x02030000, 0x02030008
    game.cpu.mem_write(preferred, struct.pack("<ii", 64 * 65536, 64 * 65536))
    game.cpu.mem_write(game.actor + OFF["Actor.movementCollisionBox"], bytes((250, 6, 252, 4)))
    assert game.call("hackLib_findLanding", game.actor, preferred, output)
    assert game.cpu.mem_read(output, 8) == game.cpu.mem_read(preferred, 8)
    game.write(0x02028000, 1, 1)
    assert not game.call("hackLib_canLandAt", game.actor, preferred)
    assert game.call("hackLib_findLanding", game.actor, preferred, output)
    game.cpu.mem_write(0x02028000, bytes([1]) * 16)
    assert not game.call("hackLib_findLanding", game.actor, preferred, output)
    game.cpu.mem_write(0x02028000, bytes(16))
    game.write(preferred, 0)
    assert not game.call("hackLib_canLandAt", game.actor, preferred)
    game.write(preferred, 1024 * 65536)
    assert not game.call("hackLib_canLandAt", game.actor, preferred)
    game = Game()
    game.write(game.actor + OFF["Actor.coords"], 64 * 65536)
    game.write(game.actor + OFF["Actor.coords"] + 4, 80 * 65536)
    game.enemy(x=80, y=80, hp=1000)
    game.cast(1)
    game.set_player("currentSpirit", 100, 2)
    game.write(game.state + OFF["RingwraithState.lastSpirit"], 100, 2)
    before = game.cpu.mem_read(game.actor + OFF["Actor.coords"], 8)
    game.call("skillExpansion_cast", 0, 8)
    assert game.get_player("currentSpirit") == 75
    game.cpu.mem_write(0x02028000, bytes([1]) * 16)
    game.call("skillExpansion_completeCast", game.actor)
    assert game.get_player("currentSpirit") == 100
    assert game.cpu.mem_read(game.actor + OFF["Actor.coords"], 8) == before


def check_damage_sources():
    game = Game()
    foe = game.enemy(hp=1000)
    observed = []
    def observe():
        source = game.read(symbols["currentSource"])
        observed.append(tuple(game.read(source + OFF[f"HackLibDamageSource.{field}"], size)
                              for field, size in (("origin", 4), ("kind", 1), ("abilityRow", 1), ("depth", 1))))
    game.stub("ringwraith_kitOnHit", observe)
    game.call("skillExpansion_dealDamage", 0x02030000, game.actor, foe, 20, 5, 9)
    assert observed == [(game.actor, 5, 9, 0)]
    assert game.read(symbols["currentSource"]) == 0
    context = 0x02030020
    game.write(context + OFF["HackLibDamageSource.origin"], game.actor)
    game.write(context + OFF["HackLibDamageSource.kind"], 3, 1)
    game.write(context + OFF["HackLibDamageSource.abilityRow"], 1, 1)
    game.write(context + OFF["HackLibDamageSource.depth"], 7, 1)
    game.write(symbols["currentSource"], context)
    hp = game.read(foe + OFF["Actor.currentHp"], 2)
    game.call("skillExpansion_dealDamage", 0x02030000, game.actor, foe, 20, 6, 1)
    assert game.read(foe + OFF["Actor.currentHp"], 2) == hp
    assert game.read(0x02030000, 1) == 0
    assert game.read(symbols["currentSource"]) == context
    game.write(symbols["currentSource"], 0)
    game.stub("actor_npcGhost_trigger", observe)
    ghost = 0x02015000
    game.call("skillExpansion_ghostTrigger", ghost, foe)
    assert observed[-1] == (ghost, 4, 255, 0)
    assert game.read(symbols["currentSource"]) == 0


def check_lifecycle_and_memory():
    game = Game()
    game.cast(2)
    assert game.read(game.state + OFF["RingwraithState.unseenFrames"], 2)
    game.write(symbols["g_ActorListMain"], game.actor)
    game.set_player("heroId", 2, 1)
    game.call("skillExpansion_initHeroLoadout", 2, 0)
    assert game.read(game.state + OFF["RingwraithState.unseenFrames"], 2) == 0
    assert not game.read(game.actor + OFF["Actor.spriteFlags"], 1) & 0x20
    assert game.read(symbols["skillExpansion_runtime"] + OFF["SkillExpansionRuntime.kit"]) == 0
    game = Game()
    game.call("modRam_init")
    assert game.read(symbols["g_Heaps"] + 12) == 0x0203F000
    block = game.call("memory_zalloc", 64)
    assert symbols["g_HeapArena"] < block < 0x0203F000
    assert game.cpu.mem_read(block, 64) == bytes(64)


def check_combat_lab():
    def command(game, row, value):
        setter = game.read(symbols["DebugLabOptions"] + row * OFF["ModOptionEntry.size"] + OFF["struct ModOptionEntry.set"])
        game.call(setter, value)
    game = Game()
    game.write(symbols["g_SceneCurrent"] + OFF["SceneState.id"], 1, 4)
    spawned = []
    def spawn():
        enemy = game.enemy(hp=100)
        game.write(enemy + OFF["Actor.type"], 25, 2)
        game.write(enemy, game.read(symbols["g_ActorListMain"]))
        game.write(symbols["g_ActorListMain"], enemy)
        spawned.append(enemy)
        return enemy
    game.stub("actor_enemySpawner_spawnChild", spawn)
    command(game, 2, 1)
    game.call("debugLab_playerTick", 0)
    assert not spawned, "lab spawned outside bonus map"
    game.write(symbols["g_GameFlags"], 1 << 7)
    command(game, 1, 1)
    command(game, 2, 1)
    game.call("debugLab_playerTick", 0)
    assert len(spawned) == 3
    command(game, 4, 1)
    game.call("debugLab_playerTick", 0)
    for enemy in spawned:
        assert game.read(enemy + OFF["Actor.flags"]) & (1 << 1)
    command(game, 3, 1)
    game.call("debugLab_playerTick", 0)
    assert game.get_player("currentHp") == 500 and game.get_player("currentSpirit") == 500
    command(game, 0, 1)
    game.call("debugLab_drawTelemetry", 0)
    assert bytes(game.cpu.mem_read(symbols["g_PlayerHuds"] + OFF["PlayerHud.textBuffer"], 4)) == b"Hit "
    game.write(symbols["g_GameFlags"], (1 << 7) | (1 << 5))
    command(game, 2, 1)
    game.call("debugLab_playerTick", 0)
    assert len(spawned) == 3, "lab spawned during co-op"


def check_shared_damage_results():
    for hit, hp, damage, expected, killed in (
        (True, 100, 20, 20, False), (True, 20, 100, 20, True),
        (False, 100, 20, 0, False), (True, 0, 20, 0, False),
    ):
        game = Game()
        foe = game.enemy(hp=hp)
        game.hit = hit
        output = 0x02030000
        game.call("skillExpansion_applyRangedHitResult", output, game.actor, foe, damage)
        assert game.read(output, 1) == hit
        assert game.read(output + 1, 1) == killed
        assert game.read(output + 4, 4) == expected
    game = Game()
    foe = game.enemy(hp=100)
    game.stub("ringwraith_kitOnHit", lambda: game.write(foe + OFF["Actor.currentHp"], 0, 2))
    output = 0x02030000
    game.call("skillExpansion_applyRangedHitResult", output, game.actor, foe, 20)
    assert game.read(foe + OFF["Actor.currentHp"], 2) == 0
    assert game.read(output + 1, 1) == 0
    assert game.read(output + 4, 4) == 20, "secondary damage leaked into primary hit result"


def check_basic_casts():
    for level in (1, 5):
        for row in (0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11):
            game = Game()
            foe = game.enemy(x=16, hp=1000)
            game.active(row, level)
            if row == 7:
                doomed = game.enemy(x=24, hp=1)
                game.write(foe + OFF["Actor.currentHp"], 0, 2)
                game.cast(0)
                game.tick()
                game.write(foe + OFF["Actor.currentHp"], 1000, 2)
                assert game.summons, "Command setup"
            if row == 8:
                game.cast(1)
            if row == 10:
                game.set_player("currentSpirit", 0, 2)
            assert game.call("skillExpansion_activeCanCast", 0, row), ("cast blocked", row, level)
            hp_before = game.read(foe + OFF["Actor.currentHp"], 2)
            game.cast(row)
            if row in (0, 1, 3, 7):
                assert game.read(foe + OFF["Actor.currentHp"], 2) < hp_before, (row, "no damage")
            elif row in (2, 4, 5, 9, 11):
                field = {2: "unseenFrames", 4: "maliceFrames", 5: "breathFrames", 9: "mordorFrames", 11: "willFrames"}[row]
                assert game.read(game.state + OFF[f"RingwraithState.{field}"], 2), (row, field)
            elif row == 8:
                assert game.read(game.state + OFF["RingwraithState.unseenFrames"], 2), "Shadowstep stealth"
            elif row == 10:
                assert game.get_player("currentSpirit") == 30 + 10 * level
                assert game.get_player("currentHp") == 75


def check_gear_slots_and_stats():
    names = ("WraithMaceItems", "WraithSword1HItems", "WraithArmorItems", "WraithAccessoryItems",
             "WraithCloakItems", "WraithGloveItems", "WraithShoeItems")
    kinds = (4, 1, 8, 10, 11, 12, 13)
    stock = (10, 11, 24, 12, 9, 8, 7)
    counts = (12, 12, 24, 15, 12, 12, 12)
    checked = 0
    for name, kind, first, count in zip(names, kinds, stock, counts):
        for index in range(first, first + count):
            game = Game()
            item = item_word(kind, index)
            slot = game.call("wraithGear_getEquipSlot", item, 0)
            assert slot < 8 and slot != 2, (name, index, "invalid/single weapon slot", slot)
            before = [game.read(game.player + OFF["Player.stats"] + i * 2, 2, True) for i in range(90)]
            game.call("item_applyAffixStats", item, slot, 0, 0)
            record = game.cpu.mem_read(symbols[name] + index * OFF["ItemStatRecord.size"], 12)
            expected = {}
            for stat_offset in (3, 5, 7, 9):
                stat = record[stat_offset]
                value = int.from_bytes(record[stat_offset + 1:stat_offset + 2], "little", signed=True)
                if stat < 90:
                    expected[stat] = expected.get(stat, 0) + value
            for stat, value in expected.items():
                actual = game.read(game.player + OFF["Player.stats"] + stat * 2, 2, True)
                if stat in (22, 23) and any(primary in expected for primary in (17, 19, 21)):
                    assert actual >= before[stat] + value, (name, index, stat, actual)
                else:
                    assert actual == before[stat] + value, (name, index, stat, actual, before[stat] + value)
            game.call("item_applyAffixStats", item, slot, 0, 1)
            for stat in expected:
                actual = game.read(game.player + OFF["Player.stats"] + stat * 2, 2, True)
                assert actual == before[stat], (name, index, "unequip", stat, actual, before[stat])
            checked += 1
    print(f"  {checked} wraith bases: equip slots, native stat grants and removal")


def check_breath_miss():
    game = Game()
    game.enemy(x=16)
    game.hit = False
    game.cast(5)
    game.tick()
    assert game.get_player("currentHp") == 100, "Black Breath healed from a missed hit"


def check_breath_overkill():
    game = Game()
    game.enemy(x=16, hp=1)
    game.cast(5)
    game.tick()
    assert game.get_player("currentHp") == 100, "Black Breath healed from overkill damage"


def check_will_new_wave():
    game = Game()
    game.cast(11)
    foe = game.enemy(x=200, hp=1)
    game.call("skillExpansion_applyPveRangedHit", game.actor, foe, 10)
    game.tick()
    assert len(game.summons) == 1, "Will of Sauron missed an unmarked new-wave kill"


def check_no_living_man_drain_gear():
    game = Game()
    game.equip(6, item_word(13, 7))  # Grave Greaves: +5% Drain Healing
    game.call("wraithGear_refresh", 0)
    game.cast(4)
    foe = game.enemy(x=16, hp=1)
    game.call("skillExpansion_applyPveRangedHit", game.actor, foe, 10)
    assert game.get_player("currentHp") == 142, "No Living Man ignored Drain Healing gear"


def check_low_life_drain():
    game = Game()
    game.equip(1, item_word(1, 11))  # Grave Blade: 1% Life Drain
    game.call("wraithGear_refresh", 0)
    foe = game.enemy(x=16, hp=1000)
    for _ in range(10):
        game.call("ringwraith_kitMeleeDamage", 0, foe, 10)
        game.call("ringwraith_kitOnHit", 0, game.actor, foe, 0, 1, 10)
    assert game.get_player("currentHp") > 100, "small Life Drain bonuses never healed"


def check_passives():
    # The 18 slots are the kit's own IDs, including the swapped-page shadow arrays.
    game = Game()
    foe = game.enemy(x=16, hp=1000)
    game.passive(0, 2)  # Servant of the Ring
    game.set_player("currentSpirit", 0, 2)
    game.call("ringwraith_kitOnHit", 0, game.actor, foe, 1, 0, 10)
    assert game.get_player("currentSpirit") == 12

    game = Game()
    game.put_gear(1, 1, 11)
    game.passive(1, 5)  # Shadow Hunger doubles drain healing
    foe = game.enemy(x=16)
    game.call("ringwraith_kitMeleeDamage", 0, foe, 100)
    game.call("ringwraith_kitOnHit", 0, game.actor, foe, 0, 1, 100)
    assert game.get_player("currentHp") == 102

    game = Game()
    foe = game.enemy(x=16)
    game.passive(2, 5)  # Wraith Form
    game.write(symbols["ringwraith_kitEnemyAttacker"], foe)
    game.call("ringwraith_addHp", 0, -100, 0)
    assert game.get_player("currentHp") == 30
    game.set_player("currentHp", 200, 2)
    game.write(symbols["ringwraith_kitEnemyAttacker"], 0)
    game.call("ringwraith_addHp", 0, -100, 0)
    assert game.get_player("currentHp") == 80

    game = Game()
    foe = game.enemy(x=70)
    game.passive(3, 4)  # Fear's Reach
    game.tick()
    assert game.get_dread(foe) > 0

    game = Game()
    foe = game.enemy(x=16)
    game.passive(4, 5)  # Lord of Terror
    game.call("ringwraith_kitMeleeDamage", 0, foe, 10)
    game.call("ringwraith_kitOnHit", 0, game.actor, foe, 0, 1, 10)
    assert game.get_dread(foe) == 45

    game = Game()
    foe = game.enemy(x=16)
    game.passive(5, 5)  # Witch-king's Mace
    game.dread(foe, 50)
    assert game.call("ringwraith_kitMeleeDamage", 0, foe, 100) == 130
    game.dread(foe, 50)
    assert game.call("ringwraith_kitMeleeDamage", 0, foe, 100) == 160

    game = Game()
    game.passive(6, 5)  # Morgul Sorcery
    foe = game.enemy(x=16, hp=1)
    game.cast(0)
    assert len(game.summons) == 1
    assert game.read(game.summons[0] + OFF["Actor.as"] + OFF["GhostState.lifetime"], 2) == 1500

    game = Game()
    game.passive(7, 5)  # Undying Malice
    game.cast(4)
    assert game.state_value("maliceFrames") == 525

    game = Game()
    game.passive(8, 1)  # Ringbound
    game.set_state("lastSpirit", 65535)
    game.tick()
    assert game.get_player("currentSpirit") == 500
    game.set_player("currentSpirit", 0, 2)
    game.cast(2)
    assert game.get_player("currentSpirit") == 15

    game = Game()
    game.passive(9, 5)  # Fading
    game.set_player("currentSpirit", 0, 2)
    game.cast(2)
    assert game.state_value("unseenFrames") == 330
    assert game.get_player("currentSpirit") == 15

    game = Game()
    game.passive(10, 5)  # Host of Angmar
    assert game.fill_thralls() == 7

    game = Game()
    foe = game.enemy(x=16)
    game.passive(11, 5)  # Dread Lord
    game.dread(foe, 50)
    assert game.call("ringwraith_kitRangedDamage", 0, foe, 100) == 120
    game.dread(foe, 50)
    assert game.call("ringwraith_kitRangedDamage", 0, foe, 100) == 140

    game = Game()
    foe = game.enemy(x=16)
    game.passive(12, 5)  # Feast of Fear
    game.set_player("currentSpirit", 0, 2)
    game.dread(foe, 100)
    game.call("ringwraith_kitOnHit", 0, game.actor, foe, 1, 0, 10)
    assert game.get_player("currentHp") == 150

    game = Game()
    game.passive(13, 5)  # Grave Pact
    game.enemy(x=16, hp=1)
    game.cast(0)
    assert game.read(game.summons[0] + OFF["Actor.as"] + OFF["GhostState.lifetime"], 2) == 900

    game = Game()
    game.passive(14, 5)  # Knife in the Dark
    foe = game.enemy(x=16)
    game.cast(2)
    game.call("ringwraith_kitMeleeDamage", 0, foe, 100)
    game.call("ringwraith_kitOnHit", 0, game.actor, foe, 1, 1, 100)
    assert game.state_value("unseenFrames") == 120

    game = Game()
    game.passive(15, 1)  # Master of the Dead
    foe = game.enemy(x=16, hp=1)
    game.cast(0)
    summon = game.summons[0]
    game.write(summon + OFF["Actor.as"] + OFF["GhostState.lifetime"], 100, 2)
    game.tick()
    assert game.read(summon + OFF["Actor.as"] + OFF["GhostState.lifetime"], 2) == 600
    assert game.call("ringwraith_kitMeleeDamage", 0, foe, 100) == 70

    game = Game()
    game.passive(16, 1)  # Death from Shadow
    foe = game.enemy(x=16)
    assert game.call("ringwraith_kitMeleeDamage", 0, foe, 100) == 60
    game.cast(2)
    assert game.call("ringwraith_kitMeleeDamage", 0, foe, 100) == 500

    game = Game()
    game.passive(17, 1)  # Endless Night
    assert not game.call("skillExpansion_activeCanCast", 0, 2)
    foe = game.enemy(x=70)
    game.tick()
    assert game.get_dread(foe) > 0
    print("  all 18 Ringwraith passive slots exercised")


UNIQUES = (
    (1, 4, 15), (1, 1, 16), (3, 8, 34), (0, 8, 35), (7, 10, 17),
    (4, 11, 14), (5, 12, 13), (6, 13, 12), (1, 4, 21), (3, 8, 46),
    (7, 10, 23), (1, 1, 22), (4, 11, 20), (5, 12, 19), (7, 10, 24),
    (3, 8, 47), (7, 10, 26), (7, 10, 25), (6, 13, 18),
)


def unique_game(index):
    game = Game()
    game.put_gear(*UNIQUES[index])
    stats = game.call("wraithGear_stats", 0)
    assert game.read(stats + 28) == 1 << index, ("unique bit", index)
    return game


def hit(game, foe, damage=100, killed=False):
    result = game.call("ringwraith_kitMeleeDamage", 0, foe, damage)
    if killed:
        game.write(foe + OFF["Actor.currentHp"], 0, 2)
    game.call("ringwraith_kitOnHit", 0, game.actor, foe, int(killed), 1, damage)
    return result


def check_uniques():
    game = unique_game(0)
    foe, other = game.enemy(x=16), game.enemy(x=24)
    hit(game, foe, killed=True)
    assert game.get_dread(other) == 40, "Witch-king's Mace"

    game = unique_game(1)
    foe = game.enemy(x=16, hp=1000)
    assert hit(game, foe) == 75, "Morgul Knife penalty"
    game.call("skillExpansion_applyPveRangedHit", game.actor, foe, 1000)
    assert len(game.summons) == 1, "Morgul Knife shard"

    game = unique_game(2)
    foe = game.enemy(x=70)
    game.tick()
    assert game.get_dread(foe) > 0, "Iron Crown radius"
    game.set_player("currentHp", 200, 2)
    game.call("ringwraith_addHp", 0, -100, 0)
    assert game.get_player("currentHp") == 85, "Iron Crown penalty"

    game = unique_game(3)
    game.set_player("currentSpirit", 10, 2)
    game.call("ringwraith_addHp", 0, -25, 0)
    assert game.get_player("currentSpirit") == 0 and game.get_player("currentHp") == 95, "Hauberk"

    game = unique_game(4)
    foe = game.enemy(x=16, hp=1000)
    for _ in range(9):
        hit(game, foe, 1)
    assert game.get_dread(foe) == 100, "Ninth Ring"
    assert game.read(game.state + OFF["RingwraithState.meleeHits"], 1) == 0

    game = unique_game(5)
    foe = game.enemy(x=16)
    game.cast(2)
    assert game.state_value("unseenFrames") == 90, "Shroud duration"
    game.call("ringwraith_addHp", 0, -10, 0)
    assert game.state_value("unseenFrames") == 90, "Shroud broken by incoming hit"
    assert game.call("ringwraith_kitMeleeDamage", 0, foe, 100) == 500, "Shroud damage"

    game = unique_game(6)
    foe = game.enemy(x=16)
    game.dread(foe, 100)
    assert hit(game, foe) == 200, "Black Captain damage"
    assert game.get_player("currentHp") == 110, "Black Captain drain"

    game = unique_game(7)
    foe = game.enemy(x=80)
    game.tick()
    assert game.get_dread(foe) == 25, "Morgul Vale"

    game = unique_game(8)
    assert game.fill_thralls() == 3, "Banner cap"
    game.enemy(x=16, hp=1)
    game.cast(0)
    assert game.read(game.summons[0] + OFF["Actor.as"] + OFF["GhostState.damage"], 1) == 15, "Banner damage"

    game = unique_game(9)
    game.enemy(x=16, hp=1)
    game.cast(0)
    other = game.enemy(x=44)
    game.tick()
    assert game.get_dread(other) == 15, "Barrow Crown aura"

    game = unique_game(10)
    game.enemy(x=16, hp=1)
    game.cast(0)
    game.cast(7)
    assert game.read(game.summons[0] + OFF["Actor.as"] + OFF["GhostState.lifetime"], 2) > 1, "Signet"

    game = unique_game(11)
    foe = game.enemy(x=16, hp=1000)
    game.cast(2)
    hit(game, foe)
    game.call("skillExpansion_applyPveRangedHit", game.actor, foe, 1000)
    assert len(game.summons) == 1, "Black Dart shard"

    game = unique_game(12)
    game.cast(2)
    assert game.state_value("unseenFrames") == 270, "Dead Marshes duration"

    game = unique_game(13)
    foe = game.enemy(x=16)
    game.dread(foe, 50)
    game.cast(8)
    game.set_player("currentSpirit", 0, 2)
    hit(game, foe, killed=True)
    assert game.get_player("currentSpirit") == 31, "Unlit Hand refund"

    game = unique_game(14)
    foe = game.enemy(x=16)
    game.set_player("currentSpirit", 0, 2)
    game.cast(1)
    assert game.read(foe + OFF["Actor.currentHp"], 2) == 92, "Horn damage"
    assert game.get_player("currentSpirit") >= 22, "Horn refund"

    game = unique_game(15)
    foe, other = game.enemy(x=16), game.enemy(x=24)
    game.dread(foe, 100)
    hit(game, foe, killed=True)
    assert game.read(other + OFF["Actor.currentHp"], 2) == 92, "Mask damage"
    assert game.get_dread(other) == 50, "Mask Dread"

    game = unique_game(16)
    foe, other = game.enemy(x=16), game.enemy(x=30)
    game.tick()
    assert game.get_dread(foe) == 24 and game.get_dread(other) == 0, "Voice aura"

    game = unique_game(17)
    foe = game.enemy(x=16)
    hit(game, foe, killed=True)
    assert game.get_player("currentHp") == 115, "Khamul heal"

    game = unique_game(18)
    foe = game.enemy(y=60)
    game.cast(3)
    assert game.read(foe + OFF["Actor.currentHp"], 2) == 79, "Saddle band"
    assert game.get_dread(foe) == 100, "Saddle terror"
    print("  all 19 unique powers exercised")


def check_killing_shards():
    for unique in (1, 11):
        game = unique_game(unique)
        foe = game.enemy(x=16, hp=1)
        if unique == 11:
            game.cast(2)
        hit(game, foe, killed=True)
        assert len(game.summons) == 1, ("killing shard", unique)


def check_ninth_ring_kills():
    game = unique_game(4)
    for _ in range(5):
        foe = game.enemy(x=16, hp=1)
        hit(game, foe, killed=True)
    assert game.read(game.state + OFF["RingwraithState.meleeHits"], 1) == 5, "Ninth Ring ignored killing hits"


def check_stun_duration():
    game = unique_game(18)
    foe = game.enemy(y=60)
    game.cast(3)
    assert game.read(foe + OFF["Actor.actionStateTimer"]) >= 75, "Fell Beast shortened terror stun"


def check_fire_weakness():
    game = Game()
    game.passive(2, 5)
    foe = game.enemy(x=16)
    flags = symbols["EnemyTypeInfos"] + OFF["EnemyTypeInfo.attackFlags"]
    game.write(flags, game.read(flags, 2) | 0x10, 2)
    game.write(symbols["ringwraith_kitEnemyAttacker"], foe)
    game.set_player("currentHp", 200, 2)
    game.call("ringwraith_addHp", 0, -100, 0)
    assert game.get_player("currentHp") == 80, "Wraith Form resisted fire-tagged melee"


def check_affixes_and_effect_removal():
    checked = 0
    for kind, base, prefix_start in ((4, 10, 156), (1, 11, 156), (8, 24, 156),
                                      (10, 12, 156), (11, 9, 36), (12, 8, 96), (13, 7, 96)):
        game = Game()
        for prefix, suffix in [(prefix_start + i, 255) for i in range(20)] + [(255, 160 + i) for i in range(16)]:
            item = item_word(kind, base, prefix, suffix)
            slot = game.call("wraithGear_getEquipSlot", item, 0)
            assert slot < 8 and slot != 2
            info = symbols["ItemTypeInfos"] + kind * 40
            records = [(game.read(info), base)]
            if prefix != 255:
                records.append((game.read(info + 8), prefix))
            if suffix != 255:
                records.append((game.read(info + 20), suffix))
            expected = {}
            for table, index in records:
                record = game.cpu.mem_read(table + index * 12, 12)
                for stat_offset in (3, 5, 7, 9):
                    stat = record[stat_offset]
                    value = int.from_bytes(record[stat_offset + 1:stat_offset + 2], "little", signed=True)
                    if stat < 90:
                        expected[stat] = expected.get(stat, 0) + value
            before = [game.read(game.player + OFF["Player.stats"] + i * 2, 2, True) for i in range(90)]
            game.call("item_applyAffixStats", item, slot, 0, 0)
            for stat, value in expected.items():
                actual = game.read(game.player + OFF["Player.stats"] + stat * 2, 2, True)
                if stat in (22, 23) and any(primary in expected for primary in (17, 19, 21)):
                    assert actual >= before[stat] + value, (kind, prefix, suffix, stat, actual)
                else:
                    assert actual == before[stat] + value, (kind, prefix, suffix, stat, actual, before[stat] + value)
            game.call("item_applyAffixStats", item, slot, 0, 1)
            for stat in expected:
                assert game.read(game.player + OFF["Player.stats"] + stat * 2, 2, True) == before[stat]
            game.equip(slot, item)
            game.call("wraithGear_refresh", 0)
            gear_stats = game.call("wraithGear_stats", 0)
            effects_before = bytes(game.cpu.mem_read(gear_stats, 32))
            assert effects_before != bytes(32), ("effects absent", kind, prefix, suffix)
            game.equip(slot, 0xFFFFFFFF)
            game.call("wraithGear_refresh", 0)
            assert bytes(game.cpu.mem_read(gear_stats, 32)) == bytes(32), "unequipped effects retained"
            checked += 1
    for unique in range(19):
        game = unique_game(unique)
        slot = UNIQUES[unique][0]
        game.equip(slot, 0xFFFFFFFF)
        game.call("wraithGear_refresh", 0)
        assert bytes(game.cpu.mem_read(game.call("wraithGear_stats", 0), 32)) == bytes(32)
    print(f"  {checked} affixed-item combinations: native stats, custom effects and removal")


def check_save_round_trip():
    for page, slot in ((0, 0), (17, 3)):
        game = Game()
        # The inventory/stat rebuild helper is separate from the slot format and skill loader.
        # Native stat rebuilding is covered by the gear tests.
        game.stub("inventory_rebuild", lambda: 0, address=0x0802ac24)
        expected_items = [0xFFFFFFFF] * 16
        for unique in (3, 4, 5, 6, 7, 9, 11):
            inventory_slot, kind, base = UNIQUES[unique]
            expected_items[inventory_slot] = item_word(kind, base)
        for inventory_slot, word in enumerate(expected_items):
            game.equip(inventory_slot, word)
        for row in range(12):
            value = row % 5 + 1 if row != 6 else 0
            if row < 6:
                game.write(game.player + OFF["Player.activeSkillLevelsPurchased"] + row, value, 1)
            else:
                game.write(game.sx + OFF["SkillExpansionPlayer.activeSkillLevelsPurchased"] + row - 6, value, 1)
                game.write(game.sx + OFF["SkillExpansionPlayer.activeSkillCycleEnabled"] + row - 6, row & 1, 1)
        for passive in range(18):
            value = 1 if passive in (8, 15, 16, 17) else passive % 5 + 1
            address = (game.player + OFF["Player.passiveSkillLevelsPurchased"] + passive if passive < 9
                       else game.sx + OFF["SkillExpansionPlayer.passiveSkillLevelsPurchased"] + passive - 9)
            game.write(address, value, 1)
        for index, row in enumerate((7, 10, 11)):
            game.write(game.player + OFF["Player.quickSelectActiveSkills"] + index, row, 1)
        slot_address = symbols["g_SaveSlots"] + slot * OFF["SaveSlot.size"]
        game.write(symbols["g_SaveSlotPage"], page, 1)
        game.write(symbols["g_SceneCurrent"] + OFF["SceneState.args"] + 2 * 4, slot)
        game.call("save_serializePlayer", slot_address, 0)
        game.call("modSave_writeSlot", slot_address, slot)
        game.set_player("heroId", 2, 1)
        for inventory_slot in range(16):
            game.equip(inventory_slot, 0xFFFFFFFF)
        for name in ("activeSkillLevelsPurchased", "passiveSkillLevelsPurchased", "activeSkillCycleEnabled"):
            game.cpu.mem_write(game.sx + OFF[f"SkillExpansionPlayer.{name}"], bytes(6 if name != "passiveSkillLevelsPurchased" else 9))
        game.call("modSave_readSlot", slot_address, slot)
        game.call("skillExpansion_loadSlot", 0)
        assert game.get_player("heroId", 1) == 8
        for inventory_slot, word in enumerate(expected_items):
            assert game.read(game.player + OFF["Player.inventory"] + inventory_slot * 4) == word, (page, slot, inventory_slot)
        for row in range(7, 12):
            assert game.read(game.sx + OFF["SkillExpansionPlayer.activeSkillLevelsPurchased"] + row - 6, 1) == row % 5 + 1
            assert game.read(game.sx + OFF["SkillExpansionPlayer.activeSkillCycleEnabled"] + row - 6, 1) == row & 1
        for passive in range(9, 18):
            expected = 1 if passive in (15, 16, 17) else passive % 5 + 1
            assert game.read(game.sx + OFF["SkillExpansionPlayer.passiveSkillLevelsPurchased"] + passive - 9, 1) == expected
        assert list(game.cpu.mem_read(game.player + OFF["Player.quickSelectActiveSkills"], 3)) == [7, 10, 11]
    print("  first/last save slots retain hero, gear, expanded skills, cycle flags and quick-select rows")


if __name__ == "__main__":
    failed = []
    for test in (check_feedback_and_large_heals, check_ability_helpers, check_shared_combat_hooks, check_cast_refunds, check_safe_landing_and_step, check_damage_sources,
                 check_lifecycle_and_memory, check_combat_lab, check_shared_damage_results, check_basic_casts, check_gear_slots_and_stats, check_breath_miss,
                 check_breath_overkill, check_will_new_wave, check_no_living_man_drain_gear,
                 check_low_life_drain, check_passives, check_uniques, check_killing_shards,
                 check_ninth_ring_kills, check_stun_duration, check_fire_weakness,
                 check_affixes_and_effect_removal, check_save_round_trip):
        if len(sys.argv) > 1 and test.__name__ not in sys.argv[1:]:
            continue
        try:
            test()
            print("PASS", test.__name__, flush=True)
        except Exception as error:
            failed.append(test.__name__)
            print("FAIL", test.__name__, error, flush=True)
            traceback.print_exc()
    if failed:
        raise SystemExit(1)
