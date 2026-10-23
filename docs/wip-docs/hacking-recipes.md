# Hacking recipes (local, learned the hard way)

## Ghidra
- Start the server: `%LOCALAPPDATA%\rotk-tools\start-ghidra-mcp.cmd` (headless, read-only, port 8080). Start it before the Claude session so the MCP tools load.
- If the MCP tools show ECONNREFUSED but the server is running, use the CLI client:
  `python -I "$LOCALAPPDATA/rotk-tools/gh.py" get_pseudocode '{"function":"0x08041f78"}'`
  `python -I "$LOCALAPPDATA/rotk-tools/gh.py" --list` lists tools.
- Function addresses: `config/functions.cfg`. Data symbols: `build/game_symbols.inc`.
- Undefined code can't be decompiled headless. Add a placeholder row to functions.cfg and rerun `make ghidra` (stop the server first).

- Many ROM functions are not defined in Ghidra. Then objdump the ROM:
  `arm-none-eabi-objdump -D -b binary -m arm -M force-thumb --adjust-vma=0x08000000 --start-address=X --stop-address=Y rotk.gba`

## gdb harness (mGBA)
- `shot.ps1` must pick the newest mGBA (`Sort-Object StartTime -Descending`), else with the user's own
  mGBA open it screenshots the user's game. VRAM/palette dumps go through gdb so they are always ours.
- `local/gdb-scripts/server.py` + `g.py`. mGBA runs with `-g` (gdb stub on port 2399). Set `PYTHONIOENCODING=utf-8`.
- `taskkill` of mGBA.exe kills every instance, including another session's. Copy a new ROM only after mGBA is closed.

## Modcode RAM budget
- `MODCODE_BSS` starts at 0x0203f5a0 and runs to 0x0203ffff (`hacks/layout.cfg`). It holds all patch globals. Overflow fails the link with `modcode .bss exceeds its RAM budget`. Check the start with `arm-none-eabi-nm build/hack.elf | grep modcode_bss`. The test-arena build is the tightest, so build it too (`make test-arena`).
- The slice comes from the top of the game heap (`g_HeapArena` 0x02002640). `mod_ram.c` calls `memory_addHeap(0, g_HeapArena, __modcode_bss_start)`, so every byte given to .bss is a byte taken from the heap. The budget start moved down by 64 B on 2026-10-23 so the test arena links. The heap only shrinks when a build actually uses that room: the main build still starts .bss at 0x0203f5e0, so its heap is unchanged.
- Heap headroom: `memory_diagnostics.c` tracks `peakTop` (the highest heap byte ever allocated) and `heapEnd`. Debug page > Allocations shows `Room:` (heapEnd - peakTop). Expect a few hundred bytes. If it nears 0, stop growing .bss.
- Guard: `modRam_bssGuard` (2 words, first in .bss via `.bss.guard` in hack.ld) is armed at boot. A heap overrun into .bss breaks it, and Allocations then shows `GUARD!`. The test arena reports it as `bssGuardIntact`.
- Hero page 2 state goes in the shared block `skillExpansion_heroState[2][SKILL_EXPANSION_HERO_STATE_WORDS]` (skill_expansion_elite.h, 88 B per player), not a new static array. Each hero file's `state(p)` returns `skillExpansion_heroStateClaim(p, HERO_ID_X, PLAYER(p).heroId)`, which zeroes the block when its owner hero changes.
- Trap: call `state(p)` only AFTER the `if (!isX(p)) return;` guard. Damage, kill and HUD hooks run for every player, so a fetch at the top of the function counts as misuse (`skillExpansion_heroStateMisuse`, arena field `heroStateMisuse`). If the guard reads state, split it into a hero check followed by a state check (see `skillExpansion_legolasDamage`).
- The heap is empty mid-mission. The peak during loads and menus is not measured. One load touched memory 656 bytes from the top. Measure the live-bytes peak before growing .bss.
- Prefer `const` tables (ROM) and small per-player state. Big buffers do not fit.

## Build
```
export PATH="~/AppData/Local/rotk-tools/arm-gcc/bin:~/AppData/Local/rotk-tools/armips:~/AppData/Local/Microsoft/WinGet/Links:$PATH"
cd ~/Desktop/lotr/rotk && make hack 2>&1 | grep -E "error|Error|warning"
```
Output: `build/rotk_hack.gba`. mGBA locks the ROM it has open, so copy to a new name: `Desktop/lotr/ROTK-Mod-Ringwraith-vN.gba` (latest v8, next v9).

## Hooks (hacks/hack.asm macros)
- `hookReplace site, handler`: 8 bytes (`ldr r3,=h; bx r3; pool`). Clobbers r3, so the target must take at most 3 args. Site should be 4-aligned.
- `hookReplaceR3 site, handler`: about 16 bytes, preserves r3. Use it for 4-arg functions, and only replace the whole function (no thunk back).
- Calling the original after an entry hook: write a thunk that replays the overwritten prologue and jumps to `func + 8`:
  ```
  asm(".section .text\n.balign 4\n.thumb\n.global X\n.thumb_func\nX:\n"
      " push {r4-r7, lr}\n mov r7, r10\n mov r6, r9\n mov r5, r8\n"
      " ldr r3, =func + 8\n bx r3\n .pool\n");
  ```
  Check the prologue bytes in rotk.gba first. `f0b5 5746 4e46 4546` is the common push{r4-r7,lr} + mov r7,r10 / r6,r9 / r5,r8.
- `game_symbols.ld` symbols already carry the thumb bit, so use `ldr r3, =func + 8` without `|1`.
- Repointing a literal pool word: `.org 0x0802xxxx` then `.word NewTable`.

## Extending ROM tables
- Declare a bigger `const` table in C with the stock part zero (`[STOCK_COUNT] = new rows...`). Then copy the stock rows over it in asm after the modcode incbin (`hacks/patches/ringwraith/wraith_gear_tables.asm`):
  `.org NewTable` then `.incbin "rotk.gba", Old - 0x08000000, size`.
  Then repoint the pointers.
- Check with `build/modcode_syms.inc` that the table landed at 0x09xxxxxx (ROM), not in RAM.
- armips macros have no token pasting (`Wraith%x%` fails). Pass every symbol name as its own argument.
- Declare extended tables with `[]` (not a fixed size) and `_Static_assert(ARRAY_COUNT(T) == STOCK + COUNT)`.
  A fixed size zero-fills a missing row silently. Hacks build with gcc `-std=gnu11`, so `_Static_assert` works.
- The asm stock-row copy counts (`wraithBaseTables ..., 10`) are the STOCK counts. They never change when
  wraith rows are added.

## Skill icon art
- `hacks/tools/skill_icon_art.py` (slice a mockup, build C arrays). Recipe: howto-heroes-skills.md section 7.
- Needs `uv run --with numpy`. System python has no PIL. Needs `make carve` output for the stock frame.

## Item tables
- ItemTypeInfos 0x08068f50, 40 bytes per type: baseItems 0, prefixRanges 4, prefixes 8, prefixFlags 12, suffixTierRanges 16, suffixes 20, suffixFlags 24, baseGfx 28, baseMetadata 32, u8 count 36.
- Sizes: ItemStatRecord 12, ItemGfx 28, ItemMetadata 8, AffixFlags 4.
- Direct literal refs to ItemSuffixFlags: 0x08026c40, 0x08026f4c, 0x080383dc.

## Gameplay HUD layout (dumped in game, Frodo, 2026-10-21)
- BG0 is the HUD: BGCNT 0x1d0c = prio 0, char base 3 (0x0600C000), screen block 29 (0x0600E800), 4bpp.
  BG1 shares char base 3 with screen block 28 (full-screen pattern, palette 2).
- BG0 row 18: x 0..2 portrait, x 3..10 HP bar (tiles 0x28..0x2f), x 11..18 gap (tiles 1,2,3,4 then
  h-flipped 4,3,2,1 = only a gold line on the bottom 2 pixel rows), x 19..26 Spirit bar (0x30..0x37),
  x 27..29 skill icon frame. Row 19 is the dark text strip (tiles 5..0xe, 0x50).
- Bar tile style: row 0 gold top (colors 2..4), rows 1..5 dithered fill (HP 7/8/9, Spirit 10/11/12),
  row 6 dark gold (4), row 7 near black (6). Empty fill is color 6.
- HUD meter tiles: 0x62..0x6a (HACKLIB_HUD_FREE_TILE). VRAM dumps in gameplay (2026-10-23, Aragorn,
  through a full herb count) show 0x62..0xff blank and unreferenced. Char tiles stop at 0xff because
  screen block 28 starts at 0x0600E000.
- 0x4d..0x55 look free but are NOT: the compass herb count (PLAYERHUD_FLAG_HERB_COUNT, also on Herbal
  Healing at full HP) and item count (PLAYERHUD_FLAG_COMPASS_COUNT) draw digits into 0x4d..0x4f and
  point compass column 28 at them for ~20 HUD ticks. A meter there garbles both.
- BGCNT in gameplay: BG0 0x1d0c, BG1 0x1c03 (char base 0, not 3), BG2 0x1e09, BG3 0x1f02.
  0x0f..0x27 are blank too but may belong to the text area, avoid them.
- BG palette bank 0 = HUD colors (enum HackLibHudColor in hacks/lib/hud_meter.h). Bank 1 = skill icon
  panel (8..13 swapped by skillExpansion_hudUpdateSkillPalette). Bank 2 = BG1 pattern. Banks 3..15 are
  used by the level tiles, so a meter has only bank 0 colors (no violet, no true black) unless you
  find a free slot per level.
- Video scroll registers are write-only (gdb reads garbage). HUD scroll is 0.
- The top-right Eye of Sauron is an OBJ (stock corruption indicator), not BG.

## Stock corruption (Burden) system (RE 2026-10-21)
Useful for any hero resource that rides on corruption, or a new meter built the same way.
- Meter: `Player.corruption` u8 0..100. Change it only through `player_addCorruption(playerIndex, delta)`
  @0x0802623c (src/c/player_addCorruption.c). Gains scale by STAT_CORRUPTION_RESIST_PERCENT (negative
  amplifies), a gain above 5 plays SFX_CORRUPTION_GAIN, and it re-evaluates the global tier flags.
- Tiers (GAME_FLAG_CORRUPTION_TIER_*): 51+ tier 1 (enemy buff in combat.c), 81+ tier 2, 91+ tier 3
  (the Nazgul hunt). Reset to 0 on mission load (scene/gameplay.c).
- Stock callers (Ghidra xrefs to 0x0802623c):
  - `skill_active_frodoTheOneRing` @0x080204a4: +50 to EVERY active player per cast (coop shares it).
  - `actor_player_tickTail` @0x0802536a: the drain, -1 each time a countdown hits -1. The countdown is
    a u16 at `*(u8 **)g_PlayerHuds[i].field_0x6c + 0x54`, refilled to 25 (0x19) frames, so about
    1.2/s at 30 fps (2.4/s at 60). It only drains while no player wears the Ring (`ringActor` NULL;
    coop checks both players). The countdown is skipped entirely while corruption is 0.
  - `actor_player_create` @0x08023484 (spawn init).
  That is all: stock Frodo gains Burden ONLY from The One Ring. Without hero code the meter drains to 0
  in under a minute, so any "stay clean" mechanic is free unless the hero adds pressure.
- Stopping or reshaping the drain without patching stock: each hero tick, write the countdown back to
  25 (it never fires), then apply your own rules. Guard the `field_0x6c` pointer for NULL. Frodo does
  this in `tickBurdenPressure` (skill_expansion_frodo.c).
- Ring state: `Player.ringActor` non-NULL while the stock Ring is on. The cast sets actor flag 0x800000
  and actionStateFlags bit 8 on the player (invisibility) and adds its duration to stats[0].

## Enemy identity (for "vs Nazgul", "vs orcs" mechanics)
- `EnemyTypeInfos[actor->unitType].family` (include/enemy.h `EnemyFamily`): NONE, ANIMALS, CREBAIN,
  WARGS, ORCS, URUKHAI, NAZGUL. Stock uses it for the STAT_DAMAGE_TO_* bonuses (combat.c) and a Nazgul
  check in combat_resolve.c.
- Specific types: `ENEMY_TYPE_NAZGUL` 58, `ENEMY_TYPE_FLYING_NAZGUL` 69. Per-type sfx in `EnemySfxSets`.
- Incoming damage attacker: `hackLib_currentDamageSource()->origin` inside the damage path (see
  `skillExpansion_addHp` in skill_expansion_elite.c, which feeds `skillExpansion_frodoIncomingDamage`).

## Skill tree screen layout (dumped 2026-10-21)
- BG0 sb 31 (banks 0,2,3), BG1 sb 30 (frames, banks 0,2), BG2 sb 29 (icons bank 4, levels 5, unique 6,
  disabled 7/8), BG3 sb 28 cb 2 (backdrop). BG palette banks 9..15 are empty here.
- gfx layer state is at 0x03003270 + layer * 0x70. Its first word is the map address, and for the tree
  that is VRAM (no RAM shadow map), so writing map entries directly is safe.
- Per-tile colors need a per-tile bank: see howto-heroes-skills.md section 7 "Palette banks".

## HUD meter (hacks/lib/hud_meter.c)
`hackLib_drawHudMeter(&HackLibHudGapMeter, value, max, &look, marks, markCount)` draws a stock style
bar into its own tiles and points the map at them. Stateless: call it every HUD tick, after
`skillExpansion_original_player_tickHud` (stock HUD redraws would cover it otherwise). Example and
hook: `skillExpansion_frodoDrawBurdenBar` called from `skillExpansion_tickHud` (skill_expansion_runtime.c).
Recipe for another hero's meter:
1. Write `<hero>DrawXBar(playerIndex)`: return unless the player is that hero, pick a
   `HackLibHudMeterLook` (light/mid/dark fill + notch color, all bank 0 indices) per state, call the
   drawer. Flash/pulse = swap looks on a frame counter in the hero state (see Frodo).
2. Call it from `skillExpansion_tickHud` next to the Frodo call (it already gates on gameplay, active
   player, no overlay).
3. One meter per hero fits the gap (8 tiles of the 9 free). A second meter at the same time needs
   another strip: shrink both to 4 tiles each (`HackLibHudMeter{x, y, 4, 0x62}` and `{x+4, y, 4,
   0x66}`) or take tiles from 0x6b..0xff (dumped blank, see "Gameplay HUD layout").
Traps: the drawer writes VRAM directly (no DMA queue), fine in the HUD tick. Only draw in
SCENE_ID_GAMEPLAY without overlay, the menus reuse char block 3. Hidden meter = stop calling it, but
then the map keeps our tiles until the HUD reloads, so restore gap tiles 1..4 (h-flip 0x400 for the right
half) if a meter ever needs to disappear mid-mission.
## Editing files from the shell
- Bash heredocs break on complex quoting. For multi-site edits, write a Python script to the scratchpad with
  `rep(path, old, new)` (assert the old text occurs exactly once) and run it with `python -I`.

## Performance (lag frames)
- Why: the game ticks at 30 Hz. rotk_main runs `game_tickFrame`, then waits until at least `*(u8*)0x03004140` (= 2) vblanks passed since `g_EngineState->lastTickVblank`. A tick that needs more than 2 frames of CPU (about 560k cycles) waits for a 3rd vblank, so the game runs at 20 Hz until the load drops. mGBA emulates the GBA CPU at real speed, so the fix has to make the ROM cheaper, not the PC faster.
- Hard rule for perf work: no behavior change. Do not skip frames, lower update rates, cull effects, cap actors, or change AI/collision results. Only memory timing, codegen/placement, or caching that gives bit-identical results.
- Where the time goes: `scene_gameplay_tick` 0x08031e44 -> `actor_tickAll` 0x080016f8 -> `actor_tickAllInList` 0x08000708, which runs `actor_tick` per actor, then the collision pass and draw-order sort, then `sprite_commitGfx` + `sprite_composeOam` per visible actor. All of that is Thumb in ROM, except two routines the game already copies to IWRAM (ARM) at boot in the undefined function at 0x0800064c: collision 0x0800cd5c -> 0x03002d30 (0x137 words, called via `_call_via_r7` at 0x080007b4) and draw sort 0x0800e208 -> 0x03002c30 (0x3e words, `_call_via_sl` at 0x080007ee). Cost scales with actor count, so many enemies/projectiles/fx = lag.
- WAITCNT: rotk_main's first line is `WAITCNT = (WAITCNT & 0xFFE3) | lit`. Stock lit 0x4014 (WS0 3/1 + prefetch), mod_save changes it to 0x4017 (+ SRAM 8 cycles). So the usual "set 0x4317" tip gains nothing here: prefetch is already on, and WS2 only matters for the EEPROM, which mod_save no longer uses (SRAM saves, EEPROM driver trapped).
- `fast_waitstates` patch: `fastWaitstates_apply` (boot task in mod_startup `runTasks`) sets WS0 first access 3 -> 2 cycles, result 0x401B. Every non-sequential ROM access (branch targets, literal pools, ROM tables) gets 1 cycle cheaper. SRAM wait (bits 0-1) and WS1/WS2 are kept. Pure timing, game logic unchanged. Only RNG seed (vblankCounter at main menu confirm) depends on timing, and that is already human-timed.
  - Caveat: 2-cycle WS0 is out of spec for Nintendo mask ROMs. mGBA and most flashcarts handle it. If a real cart shows glitches, comment `fast_waitstates` out of patches.cfg.
  - Verify: `make hack HACKS="mod_save mod_ram mod_scenes mod_startup save_slot_pages ranger_hollow_pages fast_waitstates" HACK_OUT=build/rotk_hack_fastws.gba`, then in mGBA Tools > I/O viewer, WAITCNT (0x04000204) should read 0x401B after the main menu shows. Compare a busy fight with and without the patch (mGBA "Show FPS" counts emulated frames, so judge by game speed or by counting ticks: watch `g_FrameCounter` vs real time in the memory viewer). Save and reload a slot to confirm SRAM still works.
- Hack build: modcode is already `-O2 -mthumb -mlong-calls` from ROM at 0x09xxxxxx (WS0 region, same timing as game code). `-mlong-calls` is required (BL cannot reach 0x08 <-> 0x09 reliably, ROM -> IWRAM not at all).

### Further perf ideas (not done, all must stay bit-identical)
1. (ringwraith, per frame) `wraithGear_refresh` in `ringwraith_kitTick` (ringwraith/ringwraith_kit.c ~line 796, body in ringwraith/wraith_gear.c `wraithGear_refresh` ~line 633) memsets and rebuilds gear stats from all equipment slots every frame. Cache the equipped `Item` words per player and rebuild only when one differs. Identical results since the stats are a pure function of those slots.
2. (ringwraith, per frame) `tickThralls` (ringwraith_kit.c ~488) walks `g_ActorListMain` once per frame via `nextThrall`, only when Master of the Dead or Grave Pact is learned. Already O(n) with an early-out, so low value. Leave it.
3. (both, per call) `skillExpansion_getPassiveLevel` (skill_expansion/skill_expansion_passive.c:27) linearly scans 18 slots, and ringwraith calls it many times per pulse/hit (`pulse`, `addDread` -> `stun`/`grantSpirit`, `maxThralls`). Identical option: cache `slot` per (player, passiveSkillId) in a small table, invalidated in every function that writes `passiveSkillIds` (`skillExpansion_initHeroLoadout`, load-slot hooks). Only worth it with profiling.
4. (ringwraith, per hit) `addDread` calls `findMark` up to twice (foe, then free slot) over 24 marks, and `dreadOf` scans again (`pulse` Black Breath / Mordor loops, `ringwraith_kitMeleeDamage`). Identical option: one pass that returns both the foe's mark and the first free mark.
5. (placement) Put hot hack functions in IWRAM as ARM (like `modSave_copy` stages its loop): needs a free IWRAM range verified in Ghidra (game IWRAM use runs to at least 0x030054f4, the stack is at the top) and a boot copy. Gain is limited to mod code time, which is small next to the stock actor loop.
6. (stock game, biggest lever, big effort) Reimplement `sprite_composeOam` 0x08002484 / `actor_tick` 0x08002050 as exact C ports compiled ARM into IWRAM, then `hookReplace` the ROM entries with jumps. Must be byte-for-byte behavior equivalent (decompile in Ghidra, port, test). Do not attempt without a matched decomp of the function.

## FX and palettes

See `fx-catalog.md`. Key addresses: FX descriptor table 0x08287b7c (stride 0x14), OBJ palette slot
tags 0x030053d0 and refcounts 0x030053e0 (0x5a = free, shared slots 9..15), slot release
0x080382a8, enemy palette ids 0x0806d27c, level-start palette load 0x08038434 (not a Ghidra
function, use objdump on extracted bytes). The FX variant argument is the OBJ palette slot.

## Corruption (Burden) across zones
Stock zeroes Player.corruption (+0x19b) in two places, both patched to `nop` in skill_expansion.asm (v18):
- `scene_gameplay_exit` + 0x18c: zone exit with SceneCurrent.args[3] != 0 (follow-the-arrow transition)
  heals HP/Spirit to max, then clears corruption.
- `actor_player_create` + 0x196: player spawn clears it, then calls player_addCorruption(p, 0)
  (that call now re-applies the tier flags for the carried value).
It is saved in SaveSlot.corruption (save_serializePlayer / save_deserializePlayer).

## Rewriting player keys (input_update wrapper)
input_update @0x08029c04 begins with push {r4-r7,lr}; mov r7,r10; mov r6,r9; mov r5,r8 (8 bytes), so it can be hookReplace'd with a thunk that replays those and jumps to input_update+8. See skillExpansion_inputUpdate in skill_expansion_smeagol.c. After the original runs, edit PLAYER_KEYS_CURRENT/PRESSED/RELEASED(p) to inject or strip keys. Skip when not SCENE_ID_GAMEPLAY or an overlay is open.

## Catching enemy projectiles
Stock projectiles are actor type 0x37. parentActor is the shooter (player actor type 0). Walk g_ActorListMain, pick type 0x37 with a non-player parent within range, hackLib_fxRemove it, then fire your own with skillExpansion_launchProjectile.

## Hero OBJ palettes (trap)
actor_applyTypeDefaults loads a player's sprite palette from ObjPalettes[0x21 + heroId], except Smeagol (heroId 7) uses ObjPalettes[0x21 + 9]. ObjPalettes[0x28] is an unused old Smeagol palette. Any tint or recolor of Smeagol must use 0x2a as its base. The engine also keeps a copy of each player's palette at 0x03005390 + playerIndex * 0x20.

## Kill counter, Rage buff, flinch, cooldown reset (Gimli Tally, 2026-10-21)
- `Player.killCount` (u16, saved in SaveSlot) is the lifetime kill count. `player_awardXp` (0x08010538) adds 1 for every active player on every enemy death. Every 500 kills it gives +2 attributePoints, sets PLAYER_STATUS_FLAG_UNSPENT_POINTS on the active player and the HUD skill panel to DRAW_UNSPENT_POINTS when idle. To add bonus kills, copy `skillExpansion_gimliAddTally` (it replicates the reward). Moria unlocks at killCount > 1999.
- Per-kill effects that should see every kill (co-op partner, abilities, summons) read the killCount delta in the hero tick (Gimli `lastKills`). Kills by your own hit come from `onKill` in resolveMeleeHit/resolveRangedHit.
- Dwarven Rage: `PLAYER(p).combatFlags.d.dwarvenRage`. The buff actor (tick `skill_active_tickGimliDwarvenRage`, fx type 3) and aura (tick fx_tickTimedAttachment, fx type 8) have `parentActor` = Gimli and frames left in `as.combat.venomDuration` (fxType in `as.gatePanel.tileSet`). Adding to venomDuration extends the Rage (`extendRage`).
- Flinch: `skillExpansion_addHp(p, amount, flinchOnDamage)`. Clear flinchOnDamage to stop the hit-stagger (`skillExpansion_gimliFlinch`).
- Cooldown reset from a resolver: `skillExpansion_finishCast` writes the policy cooldown AFTER the resolver returns, so zero `skillExpansion_runtime[p].cooldowns[row]` on the next tick. Refund with `player_addSpirit(p, skillExpansion_runtime[p].paidSpirit)` (paid before resolve, 0 when Spirit is free).
- Skill tree A button: `scene_skillTree_handleA` (0x08042780) only calls `scene_skillTree_buySkill(activePlayerIndex)`. Hooked by `skillExpansion_treeHandleA` for kill-bought pages. Points box redraw: `font_setStyle(FONT_STYLE_21PX,3,-1); gfx_fillBgRect(3,0xe,0xb,5,4,0); font_drawString(0xdd,0x82,0x5e,text_formatInt(n,buf),1)`. Buy sfx: SFX_SKILL_TREE_BUY_ACTIVE 0x32, SFX_SKILL_TREE_BUY_PASSIVE 0x31, error SFX_MENU_ERROR.

## Legolas Overdraw (RE 2026-10-23)
- `player_tickOverdrawCharge` (0x08025cbc, C in src/c/): B held 15 frames (`bowHoldFrames`) starts state 0x60.
  Stock gate is `itemType == ITEM_TYPE_BOW` (unique bows fail). The mod replaces it with hookReplace
  (skill_expansion.asm) using the BUGFIX family test.
- State 0x60 body is at 0x08024a18 (jump table 0x080239f4). It compares Actor byte 0x22 (`previousAnimationFrame`)
  with 9 (full draw) and fires on the B release while there.
- Charge callers: 0x08023d2e (idle) and 0x080243c2 (mid-shot) only, no reset on B release (taps accumulate).
  The mod counts its own continuous hold in input instead.
- Trap: `currentAnimationFrame` is at 0x23. Check field offsets against the stock asm (`subs`/`adds` chains on
  the actor pointer) before trusting a header name for a timing test.

## Recolor a HUD bar (Gandalf's white Spirit bar, v36)
- Spirit bar fill is BG palette bank 0 colors 10..12. Read RAM at `(volatile u16 *)BG_PALETTE_RAM + 10`,
  write with `gfx_loadBgPaletteColors(staticConstColors, 10, 3)`.
- Only for `ACTIVE_PLAYER_INDEX` and `!g_GameFlags.d.overlay` (menus reload the palette).
- The HUD reloads stock colors on its own. Each tick: if RAM is neither your colors nor the saved stock, save
  RAM as stock, then write yours. Restore only if RAM still shows yours. See showHud in skill_expansion_gandalf.c.

## Hero sprite palette tint (v36)
- Hero palette is `ObjPalettes[0x21 + heroId]` (Smeagol 0x2a). Blend per channel toward a 16-color target and
  load with `hackLib_fxTintSlot(actor->oam.paletteNumber, colors, 0, 0, 0, 0)`. Refresh every 30 frames,
  since the slot can be reloaded. Gandalf colors 3..14 are robe/hat/beard, 0..2 skin, 15 outline.

## Mod options text layout (trap, 2026-10-23)
- The options screen is BG3 (char base 0x06008000). BG2 shares that charblock and uses tiles 0..0xe1 for the art.
- Row glyphs: `row * 0x19 + 0xf6`, label and value share the 25 tiles. "Mod Options" label 0x1d8, page counter 0x1e8.
- Page title: tile 0x200 (`MOD_OPTIONS_TITLE_TILE`). The stock title base 0xe2 only had 0x14 tiles before row 0,
  so 11px titles over 80 px got overwritten by row 0's label and left ghost glyphs top right.
- `drawTitle` clears the full header row (cols 0..29), or the "Page x/y" counter lingers on the stock screen.
- To find free tiles: dump 0x06008000..0x0600E000 and the four screen maps, then list tile indices no map references.
- Limits for page authors are in docs/mod-options.md (title ~14 chars, help 2 x ~26 chars, max 6 entries with help).
