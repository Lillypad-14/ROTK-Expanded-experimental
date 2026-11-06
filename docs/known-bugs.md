# Known bugs

Bugs in the original game that the mod fixes, or will fix, in this GBA release. Fixes live
in `hacks/patches/bugfixes/` unless noted. Most entries come from `BUG` notes in `src/c/`, where the
decompiled function documents the cause.

Status: **fixed** (in the mod, both editions), **open**, or **already fixed** (by another patch).

## Gameplay

| # | Bug | Where | Status |
|---|-----|-------|--------|
| 1 | Unique bows never show their arrow effects: the arrow effect pick has no unique branch (player_setHandFx has one), and equipped arrows replace the weapon's word | stock 0x08026E88 (arrow effects) | fixed |
| 2 | Unique weapons skip weapon-family bonuses (Blademaster, Axemaster, Rangemaster) in both hands | `src/c/combat_damageCalc.c` | fixed |
| 3 | Unique staves never set the main-hand staff flag | `src/c/item_statModifiers.c` | fixed |
| 4 | Dwarven Rage looks like a crit but deals normal damage, and never crits players | `src/c/combat_resolve.c` | open: left out of the GBA release, the fix changes balance |
| 5 | Corruption skips tiers on a big hit and can leave tier 3 stuck on a big drop | `src/c/player_addCorruption.c` | fixed |
| 6 | Herb pickups heal fixed amounts instead of Foraging's curves | `src/c/actor/lootPile.c` | needs a decision: Foraging says 15 HP/level, pickups give 25 |
| 7 | Fragile is only checked in a prefix's second stat slot, so with Fragile elsewhere (Obsidian, Crystal Crook of Jas Mynn) a kill roll deletes backpack slot 0 instead | `src/c/combat.c` | fixed |
| 8 | Non-Orc carried-passive uniques (Galadriel's Phial, the Evenstar, ...) never drop: the owned test is inverted; their pickup refusal also reads the weapon bits | `src/c/loot.c`, `src/c/actor/lootPile.c` | fixed |
| 9 | Equipping a two-hander can subtract the wrong item's stats | `src/c/item_equipment.c` | fixed |
| 10 | A carried-passive unique's collected bit is never cleared when it is deleted (wrong bit index) | `src/c/item_equipment.c` | fixed |

## Skills

| # | Bug | Where | Status |
|---|-----|-------|--------|
| 11 | Skill reset zeroes bought levels but keeps their stat bonuses | `src/c/skill.c` | fixed in `skill_expansion_passive.c` |
| 12 | A one-level skill can be raised past its max level | `src/c/skill.c` | fixed in `skill_expansion_passive.c` |
| 13 | Passive bonuses applied with a skill id where a tree slot is expected | `src/c/skill.c`, `src/c/skill_levels.c` | already fixed (skill_expansion replaces these) |
| 14 | Passive loops read one entry past the count; active loops walk 7 rows instead of 6 | `src/c/skill_levels.c`, `src/c/skill.c` | already fixed (skill_expansion replaces these) |
| 15 | A temporary level in slot 6 can leave an empty selection stuck until L | `src/c/skill.c` | already fixed (skill_expansion replaces these) |

## Text and UI

| # | Bug | Where | Status |
|---|-----|-------|--------|
| 16 | Lines of 64 or more source bytes overflow the line buffer | `src/c/font.c` | open |
| 17 | An empty template argument draws its NUL and runs past it | `src/c/font.c` | open |
| 18 | Item name truncation backtracks before the name, may not fit " ...", and arrow stacks show an affix separator | `src/c/item_affix_formatName.c` | open |

## Engine

| # | Bug | Where | Status |
|---|-----|-------|--------|
| 19 | The backward memory copy corrupts data when both pointers are word aligned | `src/c/memory.c` | open |
| 20 | Growing the topmost heap block dereferences a NULL next | `src/c/memory.c` | open |
| 21 | Sound init may copy over the heap arena's start (assumed, unconfirmed) | `src/c/sfx_init.c` | open |

## Already fixed by other patches

- Legolas cannot start Overdraw with unique bows: `skill_expansion` (`skill_expansion_legolas`).
- Gandalf cannot cast Sword of Power or Lightstrike with unique swords or staves: `skill_expansion`
  (`skill_expansion_active.c`).

## Open reports, not reproduced

- The Nazgul (Ringwraith) vanished on entering the Isengard campsite in multiplayer.
- Overdraw with a unique bow may draw but not shoot.
