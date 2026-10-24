# HANDOFF: Ringwraith page 2 skills + wraith gear variety

Newer audit and current-session status: [HANDOFF-ringwraith-audit.md](HANDOFF-ringwraith-audit.md).
Read its user decisions, delivered-build notes and verification limits before treating the v8 notes below as current.

Read first: `howto-heroes-skills.md` ("Rules that bite", sections 2, 5b, 7), then
`hacking-recipes.md` (Ghidra, hooks, build, performance).
Latest ROM: `Desktop/lotr/ROTK-Mod-Ringwraith-v8.gba` (2026-10-20). Next one: `-v9`.

## Status (v6)

v6 = v5 + fixes from in-game testing (gdb harness in `local/gdb-scripts/`):
- Shadow of Mordor plays `HackLibFxShadowPool` (hacks/lib/fx_sprites.c, made by
  `hacks/tools/fx_sprite.py pool` from the Witch-king's last death frame) instead of the WK death
  anim, so no body appears. fxHold now keeps the last frame for the full duration.
- Enemy anims and custom sprites take a frameTicks speed (`hackLib_fxSprite`, `hackLib_fxEnemyAnim`).
- v7: Black Breath's slowed smoke starts BREATH_SMOKE_LIFT px up (castFxRaised, detached), because
  SMOKE_RISE's first frames sit below the feet and linger there when slowed.
- Cast visuals on the wraith go through `castFx` (stock shape slowed by `hackLib_fxPace` to
  CAST_FX_TICKS), since stock shapes step every 1 or 2 ticks and flash by.
- Morgul Blade flares the blade: `hackLib_fxTintSlot` mixes palette colors 8..12 (the blade ramp)
  of the wraith's slot toward pale green and back over BLADE_GLOW_FRAMES. Untested in game.
- Verified in game: Mordor pool, Morgul Blade, Shadowstep, Cry, Will, Sacrifice, Fell Beast.
  Not seen yet: blade glow, Command of the Nine fire (needs thralls).
- Modcode .bss (2 KB, hacks/layout.cfg) is nearly full. Growing it shrinks the game heap, which
  loads touch almost to the top. Measure the live-bytes peak before growing it.

## Status (v5)

v5 = v4 + visual polish pass: every wraith skill has its own effects (map and risks in
`fx-catalog.md`, code in `hacks/lib/fx_kit.c`). Built cleanly, NOT tested in game.

## Status (v4)

Built cleanly, NOT yet tested in game. Everything below is implemented.

- Page 2: 5 actives (rows 7..11) and 9 elite passives (slots 9..17), three builds.
- Custom icons for all 15 wraith skills from the user's art (`hacks/patches/ringwraith/icons/*.png`,
  generated `ringwraith_icons.c/.h`). Image to skill mapping is in the PNG file names.
- Gear: 8 new tiered base families (40 bases), 11 new uniques, 4 prefixes, 4 suffixes, 3 effects,
  16 drop slots. Row-count static asserts in `wraith_gear.c`.
- `fast_waitstates` patch (separate agent): ROM wait 3 -> 2 cycles for less lag. See
  hacking-recipes.md "Performance".

## Skill map

| Row/slot | Skill | Code |
|---|---|---|
| active 7 | Command of the Nine | `commandOfTheNine` in ringwraith_kit.c, baseRow CRY |
| active 8 | Shadowstep | `shadowstep`, baseRow UNSEEN_WORLD |
| active 9 | Shadow of Mordor | `shadowOfMordor` + pulse processing, baseRow FELL_BEAST |
| active 10 | Black Sacrifice | `blackSacrifice`, baseRow MORGUL_BLADE |
| active 11 | Will of Sauron | `willOfSauron`, baseRow NO_LIVING_MAN |
| passives | Fading, Host of Angmar, Dread Lord, Feast of Fear, Grave Pact, Knife in the Dark, Master of the Dead (kst), Death from Shadow (kst), Endless Night (kst) | `passiveLevel(p, SKILL_EXPANSION_PASSIVE_X)` reads in ringwraith_kit.c |

Defs are exported from `ringwraith_kit.c` (`RingwraithCommandOfTheNine` ...) and listed in
`SkillExpansionActiveDefs[HERO_ID_RINGWRAITH]` in RingwraithRow order. Passives are in
`skill_expansion_content.c`.

## New uniques (gear slot: effect)

| Unique | Slot | Effect | Build |
|---|---|---|---|
| Banner of Angmar | maul | +1 thrall, +3 thrall damage | Thralls |
| Crown of the Barrow-king | helm | foes near thralls gain Dread | Thralls |
| Carn Dum Signet | ring | Command of the Nine spares one thrall | Thralls |
| The Black Dart | fang | Unseen blows plant a shard | Assassin |
| Shroud of the Dead Marshes | wrap | Unseen World 50% longer | Assassin |
| The Unlit Hand | grips | Shadowstep-strike kill refunds its Spirit | Assassin |
| Horn of Minas Morgul | pendant | Cry double damage, refunds half | Fear |
| Mask of Dol Guldur | helm | terrified foes burst on death | Fear |
| Voice of Sauron | pendant | half aura radius, double aura Dread | Fear |
| Ring of Khamul | ring | kills heal 3% | Generic |
| Fell Rider's Spurs | treads | Fell Beast twice as wide, terrifies | Generic |

## Unverified assumptions (Ghidra was down while writing them)

Check these first if thralls misbehave in game:
1. The ghost NPC code decrements the owner's `comboState[0]` when a ghost despawns.
2. Setting `ghostState(g)->lifetime = 1` despawns a thrall next tick (Command of the Nine, Grave
   Pact expiry burst).
3. Thrall hits go through `skillExpansion_applyPveRangedHit` with the owner as attacker (Dread
   Lord bonus on thralls, `ringwraith_kitRangedDamage`).
4. Host of Angmar + Banner allow about 10 thralls. If the actor pool is full the spawn fails
   (no crash expected, just fewer thralls).
5. Shadowstep places the wraith at target.x +/- SHADOWSTEP_OFFSET with no wall check. It may land
   inside a wall.

## Test checklist for v4

1. Debug page: max level and skill points. Both tree pages show the right icons and texts.
2. Buy every page 2 skill and cast each active. Watch cooldowns (shared with baseRow).
3. Thralls: raise several, cast Command of the Nine, check the count recovers (assumption 1).
4. Gear: new names, icons, effect lines, unique descriptions. An old v3 save keeps its gear names.
5. Save, reload, check page 2 levels persist.

## Test checklist for v5 (visuals)

1. Shadow of Mordor: Witch-king death shadow at the zone center, stays until the zone ends.
2. Shadowstep: Nazgul spectral-fire death left at the old spot.
3. Command of the Nine: spectral fire on up to 3 thralls, then bursts.
4. Every other skill: effect shows, right color, no garbage tiles on enemies or the hero.
5. Cast several big skills in a crowded level: no crash, missing effects are acceptable.
6. Pause during an effect, unpause: colors still right.

## Ideas not done

- Stock passives on a custom hero cannot take custom icons.
- Agent perf ideas 2..6 (passive level cache, mark scan merge, IWRAM code) in hacking-recipes.md.

## v8: climbing foes

Pillar/ledge spawns (enemy spawner variant 1/2) start in behaviorState 0x2f with ACTOR_FLAG_COLLISION_CHECK cleared. The climb handler (0x0800b050) only advances while actionState is 2. Any hit mid-climb aborts it and strands the foe inside the wall, where it later gets terror-stunned. skillExpansion_isFoe now requires ACTOR_FLAG_COLLISION_CHECK. Any new AoE must filter targets with isFoe.

## Thrall hits (verified)

Ghost/thrall NPCs are `NPC_KIND_SIEGE_B` (0x1a), spawned by `spawnNpc` 0x08035864. Their collision trigger callback 0x08016354 calls `combat_applyPveRangedHit(owner, victim, ghost damage byte)`, so thrall hits run the player's on-hit wrappers with the owner as attacker (resolves assumption 3).

Gear reference: `ringwraith-gear.md`.

## Session 2026-10-24 (ROMs v12 to v24, all untested in game unless marked)

Latest ROM: `Desktop/lotr/ROTK-Mod-Ringwraith-v24.gba`. Next is -v25.

| ROM | Change |
| --- | --- |
| v12/v13 | Page 2 passives turned the menu blue after a purchase. `tintTileForBuild` tinted the shared drawn bank 4 for stock-icon tiles with no build, so the last drawn tile recolored the whole page. Those tiles now get their own bank 15 (`NO_BUILD_PALETTE`, skill_expansion_tree.c). Build banks use 9..14, icon color groups 9..14: a page mixing build-tinted stock icons with own-color icons still collides. |
| v14 to v16, v24 | Icons. All 29 wraith skills use user art from `new icons/Wraithimproved.png`, `Wraithimproved2.png`, `Wraithimproved3.png`, then `Wraithbreath.png` and `Wraithsword.png`. Page 1 passives now carry `RINGWRAITH_ICON(...)` too. See the icon notes in howto-heroes-skills.md. Backups: `Desktop/lotr/icon-backup-ringwraith*`. |
| v17 | Respawn HP (verified by the user). The wraith's `player_addHp` refuses gains, and the stock full heal (`scene_gameplay_healAllPlayers`, 0x08032434, modes 4..7) is a plain addHp(max). `ringwraith_healAllPlayers` sets `fullHeal` around it. |
| v18 | Thralls. `ringwraith_tickThrall` drops a swing when the target dies or drifts past reach + 16 px (reach is byte 0x92 of the ghost record). The stock ghost holds still until the swing animation ends. |
| v19 to v22 | Fell Beast dive. See fx-catalog.md. Four pieces, `tickFellBeast`, instant sweep fallback. |
| v20 | Burden (corruption) now clears on a save load. skill_expansion.asm removed the stock resets so Burden carries between zones, which also carried it across loads. Done in the same heal hook. |
| v21, v23 | Spirit HUD. A refused Spirit gain and `limitStartSpirit` changed Spirit without `PLAYERHUD_FLAG_SPIRIT_DIRTY`, so the bar showed the old value until the next change. |

Test checklist:
1. Open both tree pages after buying passives on page 2: colors must not change.
2. Die and reload: full health, Burden 0, Spirit bar a quarter (full with Ringbound) on the first frame.
3. Fell Beast facing left and right: size, direction (the left and right sheets may be swapped), one hit per foe, slowdown with four effect actors.
4. Kill a foe next to a thrall: it should turn on the next foe at once.
5. Every icon on both pages and in the HUD while casting.

Known limits: `make test-arena` fails the BSS link, because the mod's RAM (`MODCODE_BSS`) has no spare bytes, so the arena sweep cannot run. Thralls with no foe within about 160 px idle (stock leash).
