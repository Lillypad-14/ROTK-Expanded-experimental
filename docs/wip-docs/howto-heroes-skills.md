# How to: heroes, skills, items, loot

Step lists for the content work that keeps recurring. Low-level recipes (Ghidra, hooks, table
extension, build) are in `hacking-recipes.md`. Update this file whenever a step turns out wrong.

## Rules that bite (read before any change)

These fail silently or corrupt saves. The build catches only the ones marked (build).

1. **Append only, never insert.** Items save their base, prefix and suffix index. Inserting a row
   anywhere but the end of a wraith table changes every saved item after it. Uniques, effects and
   passive IDs are also append only.
2. **Parallel tables stay in step (build).** Each wraith base has a row in 4 tables: `*Items`,
   `*Gfx`, `*ItemFlags`, `*Effects`. Bump the `*_COUNT` define and add all 4 rows. The
   `_Static_assert` block in `wraith_gear.c` fails the build if a row count is off.
3. **Unique cap (build).** `WraithGearStats.uniques` is a u32 mask, so at most 32 wraith uniques.
4. **Page 2 actives order.** `SkillExpansionActiveDefs[hero][i]` lands on row 7 + i. Its index must
   match the hero's row enum (the Ringwraith's `RingwraithRow`), or the wrong callback runs.
5. **Drop weights sum to 100.** `WraithDropSlots` weights. Not checked by the build.
6. **Cooldown bit is shared.** A page 2 active uses its `baseRow`'s cooldown bit. Give each page 2
   active a different baseRow.
7. **New per-player state must be saved** if it should survive a reload (`skill_expansion_save.c`).
   Timers that reset on load are fine unsaved.
8. **No em-dashes in repo files.** Terse comments, one per struct field (AGENTS.md).
9. **Build, then copy to a new ROM name.** mGBA locks the open ROM.
10. **Every custom AoE, stun or Dread target goes through `skillExpansion_isFoe`** (skill_expansion_elite.c:160). It requires hp > 0, not NPC, not a player, not invulnerable, and `ACTOR_FLAG_COLLISION_CHECK`.
    Why: pillar/ledge climbers (enemy spawner variant 1/2, e.g. Moria) start in behaviorState 0x2f with collision cleared. The climb handler 0x0800b050 only advances while actionState == 2, so a hit mid-climb aborts it and strands the foe in the wall (fixed in v8). `actor_checkDeath` 0x0800a54c also clears collision when it sets dying state 0x11.
11. **Stock stun pattern** (copy it for new stuns): gate on hp > 0 and `actor_isStunnable`, then actionState 0x1a, `actionStateTimer = frames`, `field_0x64 |= 2`, `fx_attachStatusEffect(NULL, foe, frames, 0xc, 8, 0, 8)`.
12. **Modcode .bss is nearly full.** See hacking-recipes.md "Modcode RAM budget" before adding globals. Fetch hero state only after the isX guard (same section). Run the hero sweep (docs/test-arena.md) after skill work.

## Where things live

| What | File |
|---|---|
| Row/slot layout, accessors, def structs | `hacks/patches/skill_expansion/skill_expansion.h` |
| Page 2 actives + elite passives (data) | `skill_expansion/skill_expansion_content.c` |
| Elite skill callbacks + combat wrappers | `skill_expansion/skill_expansion_elite.c/.h` |
| Hero kit registry (`skillExpansion_getHeroKit`) | `skill_expansion/skill_expansion.c` |
| Skill tree page swap, tile draw, required levels | `skill_expansion/skill_expansion_tree.c` |
| HUD icon palettes | `skill_expansion/skill_expansion_hud.c` |
| Save of the new rows/slots | `skill_expansion/skill_expansion_save.c` |
| Ninth hero plumbing | `hacks/patches/ringwraith/ringwraith.c/.asm`, `ringwraith_hook.asm` |
| Ninth hero kit (worked example of a full custom kit) | `ringwraith/ringwraith_kit.c` |
| Hero-only gear (worked example) | `ringwraith/wraith_gear.c`, `wraith_gear.asm`, `wraith_gear_tables.asm` |
| Test cheats (max level, skill points) | Debug mod-options page, see memory `debug-cheats-patch` |

Layout: active rows 0..4 stock, 5 Herbal Healing, 6 empty, 7..12 page 2 (12 is optional). Passive slots 0..8 stock,
9..17 page 2. Always use the `skillExpansion_active*` / `skillExpansion_passive*` accessors, never
raw `PLAYER(p).activeSkillLevels[row]` for rows >= 6.

## 1. Add a page 2 active skill to a stock hero

1. In `skill_expansion_content.c`, write an `ActiveSkill` row: callback, spiritCost and cast fields
   copied from the stock row whose pose it borrows (see `KINGS_COMMAND_CAST_FIELDS`).
2. Write a `SkillExpansionActiveDef`: row, icon (any stock `ActiveSkill*Icon`, page 2 retints it),
   requiredLevels `{l1, l2, l3, l4, l5, -1}`, description with `@1` level and `@2` spirit, optional
   canCast, baseRow (the stock row it borrows its pose, cooldown bit and cast rules from).
3. Put it in `SkillExpansionActiveDefs[HERO_ID_X][0..4]`.
4. Callback: in `skill_expansion_elite.c` (declare it in `skill_expansion_elite.h`). Reuse helpers
   there: `stunFoe`, `skillExpansion_spawnBurstFx`, `raiseGhost`, `skillExpansion_isFoe`,
   `actor_iterateInRadius`, `findNextEnemy`. Level: `castLevel(playerIndex)`.
5. Ongoing effects (buff timers, on-hit, on-kill) go in `EliteState` plus `eliteTick` / `bonusDamage`
   / `onKill` in elite.c.
6. Alternative: set `stockCallback` to reuse a stock callback that reads baseRow's level.

## 2. Add a page 2 (elite) passive

1. Add an id to `enum SkillExpansionPassiveId` before `SKILL_EXPANSION_PASSIVE_END`.
2. Add its entry to `SkillExpansionElitePassives` with `ELITE_PASSIVE(maxLevel, firstLevel, ...)`,
   `iconPassiveSkillId` (stock icon) and description with `@1`. For custom art also set `.icon`
   (section 7). It wins over iconPassiveSkillId when non-NULL.
3. List it in `SkillExpansionPassiveIds[HERO_ID_X]` (9 per hero, `SKILL_EXPANSION_PASSIVE_ID_NONE`
   for empty).
   TRAP: the tree does NOT draw passives in list order. `skillExpansion_treeDrawPassiveGrid` sorts
   them by FIRST required level (ties keep list order) and fills the 3x3 grid row-major. For build
   columns (col 0/1/2 = the three builds) give each list entry a first level >= the previous one,
   e.g. Aragorn 10,11,12 / 13,14,15 / 17,20,21. Unequal levels shuffle skills across columns.

TEXT FIT: every description must wrap to at most 3 lines in the 208px box (font 0). Check with
`python -I hacks/tools/skill_text_fit.py hacks/patches/skill_expansion/*.c hacks/patches/ringwraith/*.c`
(exits 1 and prints the wrap of each overflow; it measures @1 as "Maxed", @2 as 3 digits).
Page 1 stock rows that a patch changes get new text in `AragornStockDescriptions` /
`FrodoStockDescriptions` (skill_expansion_content.c), read by `skillExpansion_getStockActiveDescription`.
4. Elite passives have no stat records. The effect code reads
   `skillExpansion_getPassiveLevel(playerIndex, ID)` wherever it applies (elite.c wrappers or the
   hero kit). Stock passives with stat records can also go on page 2 directly (see Frodo's list).

### Hero logic without a kit (Frodo pattern)
When page 1 stays stock, do not use a kit. Put data in skill_expansion_content.c and logic in
`skill_expansion_<hero>.c`, called from the existing wrappers in skill_expansion_elite.c (addHp, computePveDamage,
resolveMeleeHit/resolveRangedHit onHit, eliteTick, eliteReset/Cleanup, wrapStockCallbacks). Each entry point
returns early unless `PLAYER(p).heroId` matches. Charm: `actor_applyPersuasion(enemy, frames)` (0x080089ac),
charmed test `combatFlags.p & ACTOR_COMBAT_FLAG_PERSUADED`, timer `field_0x54`. Eligible: isFoe, ACTOR_TYPE_ENEMY,
variant != 0xd, no ACTOR_FLAG_NO_FLINCH. Watch baseRow cast rules in meetsHeroRequirements (Frodo Snare needs
Snare level > 0).

### Page 2 bought with kills (Gimli Tally pattern)
Gimli's page 2 is bought with Tally marks (1 per 10 kills) instead of skill points. To give another hero a non-point currency:
1. Make `skillExpansion_gimliUsesTally` (or a new `usesX(p)`) return TRUE for that hero. `tallyPage()` in skill_expansion_tree.c then routes A to `buyWithTally`, draws marks in the points box and "Tally: N kills" as the title.
2. requiredLevels of that page count in `GIMLI_TALLY_KILLS_PER_STEP` (20) kills, still <= 50 for the validator. Write "(Kills @1)" in descriptions, the tree multiplies @1 by the step.
3. Stock `scene_skillTree_buySkill` must never run on such a page, or it would also sell ranks for points (levels 1..20 pass its level check).
4. Spent marks = ranks bought on the page (activePurchased + passivePurchased of page 2). Old saves with ranks there count them as spent.
5. Effects keyed on kills read the killCount delta in the hero tick. See hacking-recipes.md "Kill counter".

## 3. Replace a hero's whole stock page (custom kit)

Follow `RingwraithHeroKit` in `ringwraith_kit.c`:
- `activeSkills[6]`, `icons[6]`, `requiredLevels[6][6]`, `descriptions[6]`, `canCast(p,row)`,
  `passiveSkillIds[9]`, `artHeroId` (whose tile art to use).
- Register it in `skillExpansion_getHeroKit`.
- Per-hero mechanics: a `<hero>_kit.c` with Reset/Tick/MeleeDamage/OnHit entry points, called from
  `skill_expansion_elite.c` under `#if HACK_<hero>` (see the `ringwraith_kit*` calls there).
- Wrap `player_addHp` / `player_addSpirit` only if the hero changes healing or damage taken (the
  Ringwraith does). Only one patch can own a whole-function hook, so extend the existing wrapper.

## 4. Add a new playable hero (PC edition)

New heroes are PC-only. Start from an enemy body, then give the hero its own kit.

1. Pick a body: `uv run python hacks/tools/new_hero.py --list` lists every enemy sprite set, its
   animations, and flags multi-piece bodies, huge sprites and fliers.
2. Scaffold: `uv run python hacks/tools/new_hero.py "Witch-king" --enemy WitchKing --base Gandalf
   --description "Lord of the Nazgul. Terror incarnate." [--group "Servants of Sauron"]
   [--portrait WitchKing] [--stats courage,defense,health,accuracy,strength] [--dry-run]`.
   The portrait defaults to the enemy's own dialog portrait, else its kind's (Orc, OrcCaptain,
   KingOfTheDead). `--remove NAME` undoes the last hero added (only the last: hero IDs are saved). It writes
   `hacks/patches/heroes/<slug>.c` (base stats only) and appends a `"pcOnly": true` entry to
   `hacks/content/heroes.json` with the enemy's palette (byte table 0x0806D27C by enemy type), its
   sounds (swing = whoosh 6/8 + enemy voice 2/8, hurt, death) and a `"body"`: every pose as data
   (`hacks/tools/hero_clips.py` documents the format). `--base` picks equipment, loot and the
   placeholder skills. Then tweak it with the tools in "Tweak a hero fast".
3. `make hack-pc`. `hero_registry.py --pc` generates every per-hero table from heroes.json: hero
   tables, stock skill rows, save identities (`(id + 1) << 8 | row`), and an empty page 2. The GBA
   build leaves PC-only heroes out. Hero select sizes itself from `HACK_HERO_COUNT`.
4. The hero now plays its base hero's stock skills with no page 2. That is a placeholder for checking
   the body, not the hero's skills.
5. Real skills: write `hacks/content/heroes/<slug>.json` (see "Skill specs" below). The registry
   gives the hero the kit compiled from it. Only unusual mechanics need a hand-written kit
   (section 3, Saruman's `ringwraith/saruman.c` is the template).
6. Test: the PC test arena (`make hack HACKS="@cfg test_arena" PC_EDITION=1 ARENA_HERO=<id>`, then
   rotk-recompiled `tools/arena.py --hero <id> --first-row 0`) casts every row headless.

### Tweak a hero fast (PC edition)

A hero's `"body"` (poses) and `"sounds"` live in the hero data region (`hacks/tools/hero_data.py`,
`HERO_DATA` in hacks/layout.cfg), and its skill spec (`hacks/content/heroes/<slug>.json`) in the hero
kits region (`HERO_KITS`, see "Skill specs"). Code reaches both only through fixed slots, so changing
them needs no rebuild. Loop: edit heroes.json or the spec, then

- `hero_check.py [hero]` (1 s): errors and warnings for every hero. Description wrap is pixel exact
  (the game's font and box), body problems (idle that walks, swing without a hit frame, recovery
  that replays an attack, cast poses with hitboxes, long hurt pose), long voices on every swing.
- `hero_preview.py <hero>` (1 s, no build): every pose as numbered frames, hit frames boxed red,
  `--gif` per pose, `--palette N` recolor, `--facing`. `--enemy X [--clip Y]` browses any enemy's
  clips with frame numbers, for picking cuts. Prints the sound row.
- `sound_preview.py`: WAVs of sfx ids, a hero's row (`--hero`), an enemy's set (`--enemy`), which
  sfx and enemies play a sample (`--find`). Mix parts: `[sfx, count]` or `[sfx, count, sample]`.
- `hero_fast.py check <hero>` (5-10 s): headless game from a savestate in a level, presses B
  (attack) once and then 3 times, writes build/preview/check_<hero>.png. `--button A` casts the
  selected skill instead (the savestate's hero must own it and have the Spirit). Make the savestate once
  per full build: `hero_fast.py state hacks/content/routes/witchking_moria.csv 3190 --save <sram>`
  (any route that ends in a level as the hero; B is attack, A the skill button).
- `hero_fast.py play`: opens the game with heroes.json's heroes, on its own save copy.
- `hero_fast.py cards` (10 s): screenshots of every hero select card (route
  `hacks/content/routes/hero_select.csv`). Card text is C data: needs the full build first.

Anything else (stats, hero select card text, C, asm) needs the full build: `make hack-pc`, then
rotk-recompiled `lab.py prepare` and `build` (about 4 minutes). hero_fast.py refuses to run when
build/rotk_pc.gba has code the prepared game lacks.

### Skill specs (PC edition)

`hacks/content/heroes/<slug>.json` (slug: the name without spaces or dashes, lowercase, e.g.
`witchking.json`) is a hero's whole kit as data. `hacks/tools/hero_kit.py` (run by the registry on
every build) checks it and writes `build/generated/hero_spec_kits.c`. The build links that apart
from the modcode, at `MEM_HERO_KITS` against `build/hack.elf`'s symbols (`hero_kit.py link`,
`hacks/hero_kits.ld`), into the hero's fixed `SkillExpansionKitSlot` (slot N is hero N). Code reaches
spec kits only through those slots (`skillExpansion_getHeroKit`, `spec_kit.c`), so `hero_fast.py`
relinks a changed spec into the prepared game in seconds. Example: `hacks/content/heroes/witchking.json`.
Traps: the kits region holds data only (no functions, no mutable state; the linker script asserts
it), since the recompiler finds code only in the modcode. Callbacks a spec names (`specKit_*`, icons,
policies) must be non-static modcode symbols. A projectile in flight keeps a pointer to its old
delivery, so savestates stay valid across spec tweaks only outside skill casts.

- `skills`: exactly 6, the first page; the 6th takes Herbal Healing's slot. `passives`: 9 stock
  passive names (`PASSIVE_SKILL_*` in include/skill.h without the prefix).
- Page 2 is all or nothing: `page2` with 5 or 6 skills plus `page2Passives` with 9 more stock
  passives (none repeated from page 1), or neither. The skill tree only offers page 2 to a hero
  with page 2 skills (`pageTwoHasSkills` in skill_expansion_tree.c), so a placeholder hero (no
  spec) has no page 2 at all. A half-filled page 2 looked broken (dark empty tiles, cursor on
  nothing), so the generator refuses one.
- Every skill: `name`, `cost` (Spirit), `levels` (5 rising hero levels), `icon` (any `*Icon*`
  symbol in `hacks/patches/*/*_icons.h`), `effect`, `cast` (quick, normal, long, ultimate), `text`
  (`{damage}`, `{amount}`, `{seconds}`, `{stun}` are filled in), optional `fx`
  (`{"shape": "SPARK", "color": "NAZGUL"}` from hacks/lib/fx_kit.h, played on the caster).
- Effects: `bolt` (projectile: `damage`, `speed`, `pierce`, `splash`, `radius`), `burst` (around
  the caster: `damage`, `radius`), `blast` (at the nearest foe within `range`: `damage`, `radius`),
  all with optional `stun` (seconds), `knockback`, `status` (poison, burn, slow) with
  `statusSeconds` and `statusPower`, `impact`/`travel` (skill_fx.json keys), `sound`. `heal`
  (`amount` HP), `ward` (`amount` percent less damage taken, `seconds`), `empower` (`amount`
  percent more melee damage, `seconds`). Per-level values are lists of 5.
- Save identities are `(hero id + 1) << 8 | (row + 1)`: never reorder a released hero's skills.
- Errors name the file and skill. Add an effect type in both hero_kit.py and spec_kit.c.

Hero select (Bonus Maps, an empty slot): `hacks/patches/heroes/hero_select.c`, a mod scene in the
Gauntlet's panel style. Groups are L/R tabs, 6 cards per page, enemy bodies drawn at 70%
(`ENEMY_BODY_SCALE`). Group, description (44 characters at most, two lines) and portrait come from
heroes.json; the registry rejects a missing portrait or a long description. Select toggles a stock
hero's Vanilla or Expanded kit. A returns to the save menu, which creates the character through its
own hero select (`heroSelect_resumeSaveMenu`). New games keep the stock Left/Right select.

Sounds: every hero has a row in the registry's hero sound table (attack and hurt sounds, 4 IDs; the
stock table has 8 rows and the game read past it for heroes 8+). A hero copies its stockHero's row
unless heroes.json gives `"sounds": [a, b, c, d]`.

Per-hero tables: stock tables have 8 rows, so heroes past Smeagol read past them. `uv run python
hacks/tools/scan_hero_tables.py` finds ROM tables indexed by Player.heroId and exits 1 on one not in
its REVIEWED list; extend a new one through the registry's table list (and `ringwraithRepoint` its
readers) or note its bounds check. It misses readers that hold the hero ID in a register for long
(the sound table was found in play), so add such finds to REVIEWED by hand.

Stats screen portrait: heroes 8+ show their heroes.json portrait (hero_portrait.c, hooked at
0x080275c8). BG3 there is 256-color, so the 4bpp dialog portrait is expanded to 8bpp in bank 6.

Limits: 16 heroes in all until the save's 4-bit hero ID is widened. The Gauntlet's hero picker and
its saved records cover the first 10 heroes only. Multi-piece bodies show only their main piece.
A body's weapon is part of the sprite, so equipped gear does not show on it.

## Page 2 slot limits
- Tree active slots are a 3/2/1 pyramid. Page 2 swaps rows 7..11 into the top 5 slots. The bottom slot
  shows row 12 if the hero's 6th new active def (`activeDefs[5]`) is non-NULL, else row 5 (Herbal) on
  both pages. `pageActiveCount()` in skill_expansion_tree.c decides it.
- To give a hero a 6th page 2 active: add the def as the 6th entry of its new-actives list, a 6th
  `SkillExpansionBuild` entry, and nothing else. Frodo's Put On the Ring is the example.
- Row 6 must stay ACTIVE_SKILL_NONE: stock code uses it as the "no skill" row.
- Saves: the 32-byte row record stores only rows 7..11 (`ROW_RECORD_ACTIVE_COUNT`). Row 12 lives in the
  ID record's `extraActives[]` (taken from reserved bytes), so old saves load with row 12 at level 0.
  Each hero's `LegacyActiveIds` row has 13 IDs (`0xN0d` for row 12).
- Trap: validate_abilities.py derives the LegacyActiveIds stride from the symbol size. If you add more
  rows, it follows automatically, but the 144-byte ID record has only 6 reserved bytes left.

## Two-form hero (Gandalf's Grey and White, v36)
- One def per row, the resolver branches on form (`isWhite(p)`). No second def table needed.
- Per-form Spirit costs: rewrite `skillExpansion_players[p].activeSkills[row].spiritCost` every tick.
  Skip it while `skillExpansion_treePage != 0` (the tree has rows 0..5 and 7..12 swapped). Discounts:
  compute from a const base table, never from the live cost, or they compound.
- Per-form icon color: return a build from `skillExpansion_activeBuild` based on the active player's form.
  New builds need a ramp pair in `BuildArtRamps` (hud.c) and the bound in `skillExpansion_buildArtRamp`.
- Input that is not a skill slot: add a `skillExpansion_<hero>Input(p)` call in `skillExpansion_inputUpdate`
  (smeagol.c). Hold-R (without L) is free: stock acts on the R press (loot) and L+R (quick slot).
- Temporary stat changes (no regen, haste): edit `PLAYER_STAT` and save the old value. Stats rebuild on load
  and level up, so re-apply each tick and only undo if the stat still holds your value.
- Every new hook in elite.c needs a stub in hacks/tests/aragorn.c, and input hooks one in hero_moves.c.

## Timing hero with nocked arrows (Legolas "The Glint", v1)
- Pattern: actives do not act on cast. The resolver stores the row ("nock") and returns SKILL_CAST_SUCCESS.
  The effect fires later from the hero tick. One shared resolver for all rows, branching on `castingActiveSkill`.
- Spirit already paid: `skillExpansion_runtime[p].paidSpirit` inside the resolver. Refund with `player_addSpirit`.
- Refusal text: `canCast` returning FALSE shows the stock message. For your own text, add a hook to
  `skillExpansion_rejectCast` (runtime.c, see `skillExpansion_legolasRejectMessage`). tests/cast_timeline.c needs a stub.
- Reading a stock move's timing: hook input in `skillExpansion_inputUpdate`, test `actor->actionState` and the frame
  the stock state body tests (Overdraw: state 0x60, `previousAnimationFrame == 9`, NOT currentAnimationFrame).
  Queue the event in input, resolve it in the tick.
- Weapon gate: test the family, not the item type (unique items have ITEM_TYPE_UNIQUE):
  `mainHand.d.itemType != ITEM_TYPE_EMPTY && ITEM_BASE_METADATA(mainHand).flags.d.familyBow`.
- Boost only stock projectile hits: in the hero's damage hook, check
  `hackLib_currentDamageSource()->kind == HACKLIB_DAMAGE_PROJECTILE`. Your own ability hits use HACKLIB_DAMAGE_ABILITY.
- Don't reuse a stock passive's name for a new passive (Legolas Keen Eyes became Elf-Sight).

## 9. HUD meter for a hero (corruption bar pattern)
See hacking-recipes.md "HUD meter" and "Gameplay HUD layout". Frodo's Burden bar is the template:
`skillExpansion_frodoDrawBurdenBar` in skill_expansion_frodo.c.

## 8. Skill visuals (FX)

Read `fx-catalog.md` before giving a skill a visual. API: `hacks/lib/fx_kit.h` (`hackLib_fxBurst`,
`hackLib_fxTimed`, `hackLib_fxAt`, `hackLib_fxEnemyAnim`, `hackLib_fxHold`). Preview shapes,
palettes and enemy anims with `hacks/tools/fx_palette.py`. Effects can fail to spawn (NULL), so
the skill logic must never depend on them.

### Large single icons and mixed sheets (2026-10-24)
- Sheets from the image generator do not have 4:3 frames. Scale by the ring: scale = ring width in pixels / 26, and use a frame origin of (ring center x - 16 * scale, ring center y - 12 * scale). `fit` only searches +-12 source pixels, so guess well. Preview before building.
- A single big icon: `slice IMG OUT --grid "X:Y:SCALE" --names name`. Wraithbreath.png used `93:26:39.5`, Wraithsword.png `35:35:38.3`.
- Sheets used for the Ringwraith (names are in the order of the sheet, row by row):
  - `Wraithimproved2.png`, 9x3: `--grid "15,224,433,642,850,1059,1268,1477,1685:123,340,562:6.5"`
  - `Wraithimproved3.png`, 8x3: `--grid "20,254,489,723,957,1192,1426,1660:110,345,575:7.33"`
- Replacing a few icons: slice only those names into the hero's icons folder (`-` skips a slot), then run `build` over the whole folder, which regroups the colors of all icons. `preview` the folder afterwards, since a shared color group can dull an icon.
