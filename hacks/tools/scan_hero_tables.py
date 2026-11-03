"""Find ROM tables indexed by a player's heroId, and flag any not yet reviewed.

Stock tables have 8 rows (one per stock hero), so a hero past Smeagol reads past them unless the
hero registry extends the table (hacks/tools/hero_registry.py) or a bounds check guards it. This
follows each heroId byte load (Player.heroId at 0x03003c00 + index * 0x1ac + 0x19f) a few
instructions to the ROM table it indexes. Heuristic: code that keeps the hero ID in a register
across many instructions (the hero sound table's readers) is missed, so it is listed by hand.

    uv run python hacks/tools/scan_hero_tables.py      # exit 1 on an unreviewed table
"""
import re, struct, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
# Every table found so far and why it is safe for any registered hero.
REVIEWED = {
    0x0801962C: "switch with cmp #7 (mission script): default case",
    0x08019840: "switch with cmp #7 (scene_credits_enter): default case",
    0x0801AB20: "switch with cmp #7 (mission script): default case",
    0x08026460: "StartItemsCases: registry",
    0x08026740: "switch with cmp #7 (actor_player_updateHandSprites): replaced for detached-hand heroes",
    0x0802A578: "BackpackCases: registry",
    0x08042340: "switch with cmp #7 (scene_skillTree_getRequiredLevel): kit heroes use their kit's levels",
    0x08043638: "switch with cmp #7 (skill_active_canCast): kit heroes use their kit's casts",
    0x0804583C: "switch with cmp #7 (scene_cutscene_enter): default case",
    0x0805858C: "SpawnOffsets: registry",
    0x080585D4: "HeroSounds: registry (found in play: missed by this scan)",
    0x0806DBAC: "LootHeroItemTypeRolls: registry",
    0x08282898: "HeroMoveAnimationBanks: registry",
    0x082830B0: "ObjPalettes: ringwraith palette hook",
}
rom = (ROOT / "rotk.gba").read_bytes()
work = Path(tempfile.mkdtemp())
END = 0x60000
(work / "code.bin").write_bytes(rom[:END])
subprocess.run(["llvm-objcopy", "-I", "binary", "-O", "elf32-littlearm", "-B", "arm", str(work / "code.bin"), str(work / "code.o")], check=True)
text = subprocess.run(["llvm-objdump", "-D", "-j", ".data", "--triple=thumbv4t", "--mattr=+thumb-mode", str(work / "code.o")],
                      capture_output=True, text=True).stdout
ins = []
for line in text.splitlines():
    m = re.match(r"\s*([0-9a-f]+):\s+(?:[0-9a-f]{4} ?){1,2}\s+(\S+)\s*(.*)", line)
    if m:
        ins.append((int(m.group(1), 16) + 0x08000000, m.group(2), m.group(3).split("@")[0].strip()))
def literal(addr, op, args):
    m = re.match(r"(r\d+), \[pc, #(0x[0-9a-f]+|\d+)\]", args)
    if op == "ldr" and m:
        target = ((addr + 4) & ~3) + int(m.group(2), 0)
        if 0x08000000 <= target < 0x08000000 + len(rom) - 4:
            return m.group(1), struct.unpack_from("<I", rom, target - 0x08000000)[0]
    return None
HERO_VALUES = {0x19f, 0x03003d9f}
found = {}
for i, (addr, op, args) in enumerate(ins):
    lit = literal(addr, op, args)
    if not lit or lit[1] not in HERO_VALUES:
        continue
    # follow ~10 instructions: the byte load gives the hero register
    hero = None
    for a2, op2, args2 in ins[i + 1:i + 10]:
        if op2 == "ldrb":
            hero = args2.split(",")[0]
            break
    if not hero:
        continue
    tainted = {hero}
    regs = {}
    j = ins.index((a2, op2, args2))
    for a3, op3, args3 in ins[j + 1:j + 14]:
        l3 = literal(a3, op3, args3)
        if l3:
            regs[l3[0]] = l3[1]
            continue
        parts = [p.strip() for p in args3.split(",")]
        if not parts or not parts[0].startswith("r"):
            continue
        srcs = set(re.findall(r"r\d+", ",".join(parts[1:])))
        if op3 in ("lsls", "lsl", "adds", "add", "muls", "mul", "mov", "movs", "subs") and srcs & tainted:
            for s in srcs:
                if s in regs and 0x08000000 <= regs[s] < 0x09000000:
                    found.setdefault(regs[s], set()).add(hex(addr))
            tainted.add(parts[0])
        elif op3 in ("ldr", "ldrb", "ldrh", "ldrsh", "ldrsb") and srcs & tainted:
            break
unreviewed = 0
for table, sites in sorted(found.items()):
    note = REVIEWED.get(table)
    unreviewed += note is None
    print(f"{table:#010x} {'OK  ' + note if note else 'NEW: review it'}  (from {', '.join(sorted(sites))})")
sys.exit(1 if unreviewed else 0)
