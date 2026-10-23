# FX catalog: skill visuals

How skill visuals work, what stock art exists, and how to make new looks. Code: `hacks/lib/fx_kit.h`
and `hacks/lib/fx_kit.c` (patch-independent, `hackLib_` prefix). Preview tool:
`hacks/tools/fx_palette.py`. Images: `fx-catalog/` next to this file.

Worked example: every Ringwraith skill in `hacks/patches/ringwraith/ringwraith_kit.c` (table at the
end). Copy its patterns for other heroes.

## Quick API

```c
#include "lib/fx_kit.h"

hackLib_fxBurst(actor, FX_SHAPE_SPARK, FX_COLOR_NIGHT, FX_LAYER_FRONT);       // one-shot, follows actor
hackLib_fxTimed(actor, FX_SHAPE_DIAMOND_RING, FX_COLOR_BLOOD, 300, FX_LAYER_BEHIND); // loops for 300 frames
hackLib_fxAt(origin, &pos, FX_SHAPE_OVAL_RING, FX_COLOR_NIGHT);              // one-shot left at a world pos
Actor *fx = hackLib_fxEnemyAnim(actor, ENEMY_TYPE_WITCH_KING, ENEMY_ANIMATION_POSE_DEATH, FX_LAYER_BEHIND);
hackLib_fxDetach(fx, &pos);                                                   // stop following, stay at pos
hackLib_fxHold(fx, 600);                                                      // freeze on last frame 600 frames
hackLib_fxRemove(fx);                                                         // despawn now
```

Every call returns NULL (or FALSE) when it cannot spawn. Always NULL-check before detach or hold.
A missing effect is never a crash, so skills must not depend on the effect existing.

A look = shape + color + layer. Stack 2 or 3 effects for a strong cast (core + ring + ground).

## Shapes (stock FX descriptor rows)

Table at `0x08287b7c`, stride 0x14: tiles, frames, palette (always 0), duration, flags. The palette
comes from the OBJ palette slot passed as the variant. Sheets: `fx-catalog/sheets/fxNN.png` (grey
ramp, every frame). Color grids: `grid_a.png` (0,3,4,5,6,7), `grid_b.png` (8,10,11,12,14,16),
`grid_c.png` (17,18,19,20,22,2). Grid columns are slots 2..8, three frames each.

| Id | Enum | Look | Good for |
|---|---|---|---|
| 0 | STAR_BLOOM | star opening into a dotted ring | hit flash, crit |
| 1 | (unnamed) | not checked, avoid | |
| 2 | SPARKLE_RING | ring of sparkles | buff applied |
| 3 | DIAMOND_RING | diamond star settling into a ring with a center | aura, timed buff |
| 4 | RISING_STREAKS | streaks rising and fanning | summon, power up |
| 5 | POOL_FLASH | small ground pool then star flash | impact on ground |
| 6 | BUBBLE_RINGS | bubbling rings, multicolor | poison, heal |
| 7 | GROUND_POOL | ground pool spreading then an outline ring | shadow, portal, summon base |
| 8 | TENDRIL_BURST | tendril rising then specks | drain, curse |
| 9 | CHECKER_RING | checkered ring | ward |
| 10 | SMOKE_RISE | big cloud rising and scattering | vanish, end of zone |
| 11 | SMOKE_SMALL | small cloud | debuff tick on a foe |
| 12 | RING_SPARKLES | ring then rising sparkles, multicolor | heal, cleanse |
| 13 | SKY_RAIN | screen-sized fall of tiny streaks | big ultimate only, heavy |
| 14 | STREAK_RAIN | streaks falling | strike from above |
| 15 | LIGHTNING | tall vertical bolt | smite, sacrifice |
| 16 | SPARK | small four-point spark | per-foe hit marker (cheap) |
| 17 | OVAL_RING | dotted oval expanding | zone pulse |
| 18 | TENDRIL_STAR | bright core with lightning tendrils | big hit, cast core |
| 19 | WIDE_RING | checkered ring expanding wide | shout, area cast |
| 20 | SHOCKWAVE | ground arcs, multicolor | slam |
| 21 | SHOCKWAVE_ALT | second arc set | slam, landing |
| 22 | DEBRIS | debris puffs, follows parent anim frame | avoid on skills |

Descriptor flags: 15/16/17 use 0x20002 (offset anchor), 22 uses 0xff04 (anim-frame anchor), the rest
0xff01. Rows 1, 9, 13 are not previewed in a grid yet. Look at their sheet before using them.

## Colors

`HackLibFxColor` has three kinds.

### Fixed slots 2..8 (free, always loaded)

The level start (0x08038434) loads OBJ slots 0..8 with ObjPalettes[0..8]. Slots 0/1 are then the
player palettes, so never use them.

| Enum | Slot | Simple shapes look |
|---|---|---|
| FX_COLOR_WHITE | 2 | white |
| FX_COLOR_EMBER | 3 | orange-red ember |
| FX_COLOR_GREEN | 4 | green |
| FX_COLOR_BLUE | 5 | blue |
| FX_COLOR_NAZGUL | 6 | brown (Nazgul and Ringwraith body palette) |
| FX_COLOR_NIGHT | 7 | violet night (stock Nightburst) |
| FX_COLOR_GOLD | 8 | gold sun (stock Sunburst) |

Multicolor shapes (6, 12, 20, 22) use more of the palette and do not follow this column. Check the
grid. Shape 18 in slot 7 is a violet-white lightning starburst.

### Borrowed ObjPalettes (`FX_COLOR_OBJ_PALETTE(id)`)

Any ObjPalettes id. Ids 2..8 map to their fixed slot. Others claim a shared slot 9..15.

### Custom palettes (`FX_COLOR_CUSTOM(i)`, shorthands `FX_COLOR_SHADOW` etc.)

`CustomPalettes` in fx_kit.c, all palette 7 recolored. Preview: `custom_palettes.png`.

| Enum | Look | Recolor args (base 7) |
|---|---|---|
| FX_PALETTE_SHADOW | near-black violet | --hue 0.75 --sat 0.6 --val 0.45 |
| FX_PALETTE_MORGUL | sickly pale green | --hue 0.30 --sat 0.7 --val 1.15 |
| FX_PALETTE_BLOOD | deep red | --hue 0.99 --sat 1.2 --val 0.9 |
| FX_PALETTE_GHOST | cold white-cyan | --hue 0.52 --sat 0.25 --val 1.4 |

Shapes 10/11 (smoke) keep pinkish parts in custom palettes. Use fixed GREEN or NIGHT for smoke.

### Add a custom palette

1. From `rotk/`: `uv run python hacks/tools/fx_palette.py recolor OUT.png --base 7 --hue H --sat S --val V`
   (hue 0 red, .33 green, .66 blue, .75 violet). Look at OUT.png (left base, right new).
2. Paste the printed array into `CustomPalettes` and add an enum value before `FX_PALETTE_COUNT`
   in fx_kit.h (explicit value, bump COUNT). Optional `#define FX_COLOR_X FX_COLOR_CUSTOM(...)`.
3. Other base palettes work too (`--base 8` for gold-based ramps).

Ideas not made yet: ICE (hue .55 sat .5 val 1.3), TOXIC (hue .22 sat 1.3), HOLY (base 8, val 1.3),
VOID (val .25), FIRE (base 3, sat 1.2).

## Palette slot system (why borrowing is safe)

- Tag table `0x030053d0` (16 bytes): ObjPalettes id loaded per slot, 0x5a = free.
- Refcounts `0x030053e0` (16 bytes).
- `actor_applyTypeDefaults` claims slots for enemies and sets `ACTOR_FLAG_DEFAULTS_APPLIED` (1<<27).
- The release routine 0x080382a8 (from `actor_tickAllInList`) decrements the slot's refcount when
  an actor with that flag is freed, and frees slots >= 9 at zero.
- fx_kit's `claimSlot` uses the same tables. Custom palettes use tags 0x80+i so they never clash
  with ObjPalettes ids. `bindSlot` sets the flag on the FX so the game releases the slot for us.
- Palette writes go to `g_ObjPaletteSnapshot` when `GAME_FLAG_OVERLAY` is set (pause/menu), else
  `gfx_queuePaletteWrite`.

Limit: 7 shared slots, shared with enemies. A level with many enemy types may have none free. Then
the call returns NULL and the skill works without the effect. Prefer fixed colors for effects that
spawn per foe (cheap and always available). Same tag effects share one slot.

## Enemy animations as effects

`hackLib_fxEnemyAnim(on, enemyType, pose, layer)` spawns a GROUND_POOL burst in the enemy's own
palette, then swaps its animation to `EnemyAnimationBanks[enemyType][pose]` (facing 0).
`sprite_setAnimation` uploads a palette only when the anim has one. Enemy anims have NULL, so the
FX keeps the slot we picked. The FX despawns when the enemy anim ends.

Preview any enemy pose: `uv run python hacks/tools/fx_palette.py enemy OUT.png --enemy 64 --pose 2`
(prints the enemy's own palette id, `--palettes 78,6` compares).

Known good:

| Enemy | Type | Palette id | Pose | Look | Image |
|---|---|---|---|---|---|
| Witch-king | 64 | 78 (borrowed slot) | death | collapses into a ground shadow, 24 frames | `witchking_death.png` |
| Nazgul | 58 | 6 (fixed slot) | death | body consumed by blue spectral fire, 18 frames | `nazgul_death.png` |

The WK death needs palette 78. In slot 6 it looks wrong. Poses: the `ENEMY_ANIMATION_POSE_*` enum, enemy types: `ENEMY_TYPE_*` (include/).

Ideas: other bosses' death or spawn poses (Mouth of Sauron, Balrog style bosses, oliphaunt dust),
ghost army attack poses for summons, troll slam pose as a ground hit. Preview first. Big anims cost
OBJ VRAM (WK is about 100x80), so keep at most one or two alive at a time.

## Holds, detach, timed vs burst

- **Burst**: type `FX_BURST_UNK_PARAM` (0x68), tick `fx_tickAttachment`. Plays once.
- **Timed**: type `ACTOR_TYPE_UNKNOWN_50`, tick `fx_tickTimedAttachment`. Loops for N frames
  (Gimli's pattern). Good for auras matching a buff duration.
- **Detach**: clears `renderFollowTarget` and spriteFlags 0x0c, sets coords, renderCoords and the
  spawn position in `as+0x20`. Use for zones, departure marks, corpses.
- **Hold**: replaces the tick with `tickHold`, which runs `fx_tickAttachment` and sets
  `animationFramesLeft = 0xff` on the last frame so it never ends, until the countdown runs out.
  The countdown includes the play time. 6 holds max (`FX_HOLD_COUNT`). A hold whose actor got
  freed and reused is detected by its tick callback and recycled.

## Pace, custom sprites, tint (v6+)

- `hackLib_fxPace(fx, frameTicks)`: sets `animationFrameDuration` and `animationFramesLeft`. Stock shapes step every 1 or 2 ticks and flash by. Ringwraith casts use `castFx` (ringwraith_kit.c:378), a burst paced to `CAST_FX_TICKS` (3).
- `hackLib_fxSprite(on, &anim, color, frameTicks, layer)`: spawns a GROUND_POOL burst, then `sprite_setAnimation` with a custom `SpriteAnimation`. Frames must fit 2304 bytes of tiles (fx_kit.h). Returns NULL when no slot or actor is free.
- `hackLib_fxEnemyAnim(on, enemyType, pose, frameTicks, layer)` is `hackLib_fxSprite` with `EnemyAnimationBanks[enemyType][pose]` in the enemy's palette.
- `hackLib_fxTintSlot(slot, base, first, count, color, strength)`: reloads a player OBJ slot (only 0..1, `FX_TINT_SLOT_COUNT`) with base, colors first..first+count-1 mixed toward color by strength / 16. Strength 0 restores base. Morgul Blade uses it on Nazgul palette indices 8..12 (the blade ramp) for `BLADE_GLOW_FRAMES` (30), rising over 6 (`tickBladeGlow`, ringwraith_kit.c:850).
  The tinted colors live in the static `tintedSlots` buffer because `gfx_queuePaletteWrite` only queues a pointer and reads it at vblank. Never pass a stack buffer to a queued palette write.

## Traps

- **Enemy death anim shows the body.** Playing an enemy death pose as an effect draws the whole enemy collapsing. That is why the Shadow of Mordor pool is a generated sprite, not the WK death anim.
- **SMOKE_RISE (10) starts below the anchor.** Measured OAM y: frame 0 spans +5..+21, frames 1..4 reach down to +24..+29, frames 6+ are fully above. Paced slow, the first frames linger at the feet and look too low. Lift it: `castFxRaised(playerActor, FX_SHAPE_SMOKE_RISE, ..., BREATH_SMOKE_LIFT)` (14 px, detaches at the raised position, ringwraith_kit.c:384).
- **Hold:** `tickHold` clears `ACTOR_FLAG_ANIMATION_END` and sets `animationFramesLeft = 0xff` once the last frame is reached, so the effect stays on its last frame until the countdown ends, then removes it.

## Generated sprites (hacks/tools/fx_sprite.py)

`uv run python hacks/tools/fx_sprite.py pool hacks/lib/fx_sprites --preview OUT.png` writes `hacks/lib/fx_sprites.c/.h` (`HackLibFxShadowPool`).
It takes the silhouette of one enemy frame (default enemy 64 Witch-king, pose 2 death, frame 23), scales it from 0.15 to full over 8 frames, and fills it with palette indices 15 (fill), 14 (edge), 13 (sheen).
Frames keep the source frame's OAM layout (96x48, 2304 tile bytes per frame) and are LZ77 packed (2236 bytes total). Draw it with `FX_COLOR_SHADOW`.
Options: `--enemy --pose --frame --frames --start`. Rerunning with defaults reproduces the committed file byte for byte.

To add another generated sprite:
1. Add a subcommand in `main()` that builds `frames_pixels` (a list of width*height index buffers) on one source frame's OAM layout. Reuse `enemy_frame` and `frame_set_bytes`.
2. The writer currently emits only the pool. Make it emit every sprite into the same `.c/.h` pair (the docstring says the pair holds all sprites), or give the new sprite its own output pair. Otherwise a rerun drops one of them.
3. Keep tile bytes per frame at or below 2304 (the source layout decides it).
4. Play it with `hackLib_fxSprite`. Preview with `--preview` first.

## Layer

`FX_LAYER_BEHIND` (alt 0, spriteFlags 8, drawn y-1 behind the parent) for ground things: pools,
shockwaves, rings. `FX_LAYER_FRONT` (alt 1, spriteFlags 4) for sparks, bolts, smoke, cores.
`hackLib_fxAt` always uses BEHIND then detaches. Derived from disassembly of 0x08003540, not seen
in game yet.

## Not used yet (research leads)

- **Particle emitters**: `fx_allocEmitter`, 14-slot pool. kind 7/8 match the palette. Variant
  indexes RAM particle descriptors at 0x030053f0 (group*0xe0 + variant*0x20). Could give trails
  and sprays. Unverified.
- **Screen effects**: `gfx_fade*`, `gfx_alphaBlend*`, mosaic. Avoided because they fight mission
  fades and dialog. If tried, restore state on every exit path and test pause during the effect.
- **New sprite art**: done once, see "Generated sprites" below.
- Sound: reuse enemy sfx (Nazgul aggro/hit/death, WK death) for flavor. See ringwraith_kit.c.

## Ringwraith map (worked example)

| Skill / event | Visual |
|---|---|
| Dread applied | terror: TENDRIL_STAR NIGHT, fright: SPARK NIGHT |
| burstAround (thrall burst) | POOL_FLASH SHADOW behind + SHOCKWAVE_ALT NIGHT behind + triple-damage sfx |
| Raise thrall | GROUND_POOL SHADOW behind + RISING_STREAKS BLUE front |
| Shard pulse | SPARK MORGUL |
| Mordor pulse | OVAL_RING NIGHT at the zone center + SMOKE_SMALL NIGHT per foe |
| Mordor end | SMOKE_RISE NIGHT at the center |
| Black Breath pulse | SMOKE_SMALL GREEN per foe, RING_SPARKLES GREEN on self |
| Enter Unseen | SMOKE_RISE NIGHT front + GROUND_POOL SHADOW behind |
| Morgul Vale | LIGHTNING MORGUL |
| Unseen strike | TENDRIL_STAR BLOOD + STAR_BLOOM SHADOW |
| Witch-king's Mace | SHOCKWAVE NIGHT |
| Morgul Blade | TENDRIL_STAR MORGUL + POOL_FLASH MORGUL + impale sfx |
| Cry | WIDE_RING NIGHT + OVAL_RING GHOST + Nazgul aggro sfx |
| Fell Beast | SHOCKWAVE_ALT on self, STREAK_RAIN NIGHT per foe |
| No Living Man | timed DIAMOND_RING BLOOD for the malice duration + POOL_FLASH BLOOD |
| Black Breath cast | SMOKE_RISE GREEN + GROUND_POOL MORGUL + Nazgul hit sfx |
| Command of the Nine | first 3 thralls: Nazgul death anim detached at each thrall, then burst; Nazgul death sfx |
| Shadowstep | Nazgul death anim left at the old spot, SPARK BLOOD on the target |
| Shadow of Mordor | `HackLibFxShadowPool` SHADOW detached at the center, paced `MORDOR_SHADE_TICKS` (5), held for the zone duration (fallback GROUND_POOL SHADOW/NIGHT) + WIDE_RING SHADOW + WK death sfx |
| Morgul Blade glow | `hackLib_fxTintSlot` on the wraith's slot, colors 8..12 toward 0x73fd |
| Black Sacrifice | LIGHTNING BLOOD + timed RISING_STREAKS BLOOD for the sacrifice duration |
| Will of Sauron | TENDRIL_STAR GOLD + WIDE_RING EMBER + SHOCKWAVE NIGHT |

## Recipe: visuals for a new hero's skills

1. Pick a theme color per build (fixed slot if per foe, custom palette for signature casts).
2. Per skill: core (front: 18, 15, 0) + ground (behind: 7, 5, 20, 21) + optional ring (17, 19).
3. Per-foe ticks: one cheap shape (16 or 11) in a fixed color. Never a custom palette per foe in a
   loop over many foes.
4. Signature ultimate: one enemy anim (preview with `fx_palette.py enemy`), detached + held if it
   marks a zone. Fallback burst when it returns NULL.
5. Timed aura when the skill has a duration (`hackLib_fxTimed` with the same frame count).
6. Add the row to a map table like the one above in that hero's handoff doc.

## Risks to check in game (v5)

1. OBJ VRAM pressure from big enemy anims (WK). Watch for garbage tiles on other sprites.
2. Detached FX draw order (may draw over or under walls oddly).
3. Layer meaning (behind/front) guessed from disassembly.
4. Enemy death anims assumed to advance linearly through frames (not an anim script).
5. Hold countdown includes play time, so the WK pool may vanish a little early or late vs the zone.
6. Shared slots exhausted in enemy-heavy levels: effects silently missing (expected, not a bug).

## Frodo map (page 2 v2)

| Skill / event | Visual |
|---|---|
| Phial of Galadriel cast | AOE_BURST_SUN + STAR_BLOOM WHITE (paced) + GROUND_POOL GHOST held at the zone (fallback WHITE) |
| Phial pulse | OVAL_RING GOLD at the zone + SPARK WHITE per foe |
| Elbereth | TENDRIL_STAR WHITE + WIDE_RING GHOST behind (paced) + SKY_RAIN WHITE, RING_SPARKLES WHITE on the partner |
| Shadow of the Ring | TENDRIL_STAR SHADOW (paced) + SHOCKWAVE NIGHT 24 px ahead, TENDRIL_BURST NIGHT per foe |
| Sow Discord | OVAL_RING GREEN behind (paced), STAR_BLOOM GREEN per target, persuasion sfx |
| Smoke and Mirrors | SMOKE_RISE GREEN + GROUND_POOL GREEN held at the origin, SPARK GREEN on landing, dodge sfx |
| Persuaded (Riddles) | timed SPARKLE_RING GOLD for the persuasion |
| Claim It | AOE_BURST_NIGHT + SMOKE_RISE SHADOW, dark sprite tint, SMOKE_SMALL SHADOW wisps |
| Burden resisted (Will) | SPARK WHITE |
| Unwavering ready / strike | SPARKLE_RING WHITE aura, AOE_BURST_SUN on the victim |
| Weight hit at 51+ | SPARK NIGHT |
| Ring's Hunger heal | RISING_STREAKS NIGHT |
| Sting glowing hit | SPARK BLUE, sprite tint toward blue while a foe is in 72 px |
| Tricksy turn | persuasion sfx + Riddles marker |

- **Timed loop only on proven shapes.** Catch a Fish crashed ("jumped to invalid address") with `hackLib_fxTimed` on OVAL_RING (17, offset-anchor descriptor) and was switched off it (suspected, not traced). Proven with fxTimed: DIAMOND_RING, RISING_STREAKS, SPARKLE_RING. Don't attach bursts to an actor you are removing the same frame; use `hackLib_fxAt` at its coords.

## Multicolor shapes do not follow the slot hue (checked with `fx_palette.py grid`)

SHOCKWAVE (20), SHOCKWAVE_ALT (21), TENDRIL_STAR (18), DIAMOND_RING (3), RISING_STREAKS (4),
RING_SPARKLES (12) and BUBBLE_RINGS (6) use several ramps, so the slot name lies. Measured:

| Shape | WHITE 2 | EMBER 3 | GREEN 4 | BLUE 5 | NAZGUL 6 | NIGHT 7 | GOLD 8 |
|---|---|---|---|---|---|---|---|
| SHOCKWAVE 20/21 | orange+blue | grey-white | near black | orange+green | cyan | red-yellow fire | olive |
| TENDRIL_STAR 18 | green core | brown | cyan core | tan | grey | violet-white | gold |
| DIAMOND_RING 3 | red, orange ring | brown | cyan-green | blue | dark | red, blue ring | yellow-green |

A custom palette (one hue) makes them read as one color: GHOST = pure white arcs and diamond,
VENOM = lime. For fire arcs use NIGHT. Preview a palette first:
`uv run python hacks/tools/fx_palette.py recolor OUT.png --base 7 --hue H --sat S --val V --types 18,20,3`.
Single-ramp shapes (0, 2, 5, 7, 8, 9, 10, 11, 14, 16, 19) follow the slot. WIDE_RING in NIGHT/NAZGUL
is nearly invisible. LIGHTNING reads best in WHITE and EMBER (orange bolt).

`FX_PALETTE_VENOM` (bright lime): `recolor --base 7 --hue 0.24 --sat 0.8 --val 1.4`.

## Saruman map (v30)

Design rule: every spell has one signature shape nobody else uses, and plays as cast (on Saruman) ->
travel (bolt trail) -> impact (cued at the target). Colors: White = WHITE/GHOST/GOLD,
Isengard = EMBER + NIGHT, Voice = VENOM/GREEN.

Sequencer in saruman.c (state lives in SarumanState, ticked from `tick()`):
- `FxTable[FXID_*]`: shape, color, pace ticks, lift in px, optional sfx per effect id.
- `cue(p, origin, &pos, delay, id)`: play id at pos after delay frames (7 slots, a full queue or delay 0 plays now).
- `launchBolt(actor, target, trailId)`: trail effect every 2 frames from Saruman to where the target stood, 12 frames.
  Cue impacts at delay BOLT_FRAMES so they land with the bolt.
- `markFoes(origin, center, radius, id, delay, step)`: cue id on up to 4 foes, step frames apart.
Damage still lands at cast, about 0.2 s before the bolt arrives.

| Skill / event | Cast | Travel | Impact |
|---|---|---|---|
| Voice of Saruman | aggro sfx | OVAL_RING VENOM ripples at 0/8, GREEN at 16 | SPARK VENOM above each foe at 10 (daze) |
| Flame of Orthanc | SPARK EMBER | SMOKE_SMALL EMBER puffs | POOL_FLASH then TENDRIL_BURST EMBER, fireburst sfx |
| Burn tick | | | SMOKE_SMALL EMBER (pace 2) |
| Blasting Fire | cast sfx | SPARK GOLD fuse | POOL_FLASH + SHOCKWAVE NIGHT, raised SMOKE_RISE at 16 |
| Many-Coloured Ward | RING_SPARKLES WHITE | | timed DIAMOND_RING GHOST for the ward, shield sfx |
| Word of Binding | aggro sfx | | CHECKER_RING VENOM seal, GREEN seal at 12, timed SPARKLE_RING GREEN while bound |
| Orthanc's Remedy | GROUND_POOL GOLD + TENDRIL_BURST WHITE | | TENDRIL_BURST GOLD at 10, SPARKLE_RING GOLD at 20, herb sfx |
| Echo of Power | WIDE_RING GOLD | | then the echoed spell |
| Furnaces of Isengard | GROUND_POOL EMBER + NAZGUL smoke | | TENDRIL_BURST EMBER per foe at 14+6i, smoke again at 20 |
| Sharkey's Curse | aggro sfx | SMOKE_SMALL GREEN wisp | GROUND_POOL SHADOW + raised SMOKE_RISE GREEN, venom sfx |
| Curse leak hit | | | SPARK VENOM |
| Light Broken | STAR_BLOOM GOLD | | WIDE_RING per held color, 8 frames apart |
| Sharkey's Spite | GROUND_POOL SHADOW | | TENDRIL_BURST BLOOD on foes, SPARKLE_RING GREEN drain at 14 if any hit |
| Power of the White Hand | SKY_RAIN WHITE + TENDRIL_STAR NIGHT | | LIGHTNING WHITE per foe at 10+6i, SHOCKWAVE GHOST at 36 |
| White refraction | STAR_BLOOM WHITE | | timed SPARKLE_RING guard, sunburst sfx |
| Isengard refraction | SHOCKWAVE NIGHT | | POOL_FLASH per foe, SHOCKWAVE_ALT NIGHT at 12 |
| Voice refraction | SHOCKWAVE VENOM | | SPARK VENOM per foe at 6+3i, timed DIAMOND_RING VENOM 60 |
| Prism gains a color | SPARK in the color, 30 px above Saruman | | |
| Powder of Orthanc | | | STAR_BLOOM EMBER at the corpse, NAZGUL smoke at 4 |

## Gandalf map (page 2 "The Grey and the White", v36)
- Swap to White: STAR_BLOOM WHITE + WIDE_RING WHITE, SUNBURST. To Grey: SMOKE_RISE WHITE, PLAYER_DODGE.
  White idle: SPARKLE_RING WHITE every 2s. Secret Fire: SHOCKWAVE NIGHT + EMBER sparks.
- Fireworks: TENDRIL_STAR GOLD + WIDE_RING EMBER, then STAR_BLOOM pops GOLD/GREEN/EMBER. Flame of Anor:
  LIGHTNING WHITE every 40px along the facing, STAR_BLOOM GOLD on hit foes.
- Never Late mark: CHECKER_RING WHITE each second. Shadowfax: SMOKE_SMALL trail, SHOCKWAVE_ALT GHOST.
- Smoke Rings: SMOKE_RISE + OVAL_RING WHITE (fxAt, not fxTimed), SMOKE_SMALL each second. Broken Staff:
  LIGHTNING + SHOCKWAVE GHOST, SPARK WHITE on foes inside each second.
- Pipe: SMOKE_SMALL WHITE every 0.5s. Theoden: RING_SPARKLES + RISING_STREAKS GOLD.
- Fly: WIDE_RING WHITE + SHOCKWAVE GHOST. Look to My Coming: STAR_BLOOM WHITE + SKY_RAIN GOLD.
- YSNP: LIGHTNING + CHECKER_RING at the line, end LIGHTNING + SHOCKWAVE GHOST. Fall: GROUND_POOL SHADOW +
  SMOKE_RISE NIGHT. Valinor: SKY_RAIN WHITE + TENDRIL_STAR GOLD + SHOCKWAVE GHOST.
- Risk: CHECKER_RING is untested in this repo. If it looks wrong, swap to DIAMOND_RING.

## Legolas map (page 2 "The Glint", v1)
- Glint: SPARK on Legolas, color by streak (WHITE 0, GREEN 1-2, GOLD 3-4, BLUE 5), SFX_GEM_PICKUP.
  True Shot: STAR_BLOOM GOLD + SFX_COMBAT_CRIT_HIT. Nock: SPARKLE_RING GOLD + SFX_SKILL_TREE_BUY_ACTIVE.
- Piercing: SPARK WHITE every 40px along the facing, STAR_BLOOM GOLD on hit foes. Pinning: SPARK GREEN line,
  DIAMOND_RING GREEN on the pinned foe, SPARK GREEN on spread foes. Gift: SPARK GREEN line + RING_SPARKLES GREEN.
- Surf: SMOKE_SMALL WHITE at the start, SHOCKWAVE_ALT GREEN at the end. Point Blank: SHOCKWAVE GHOST.
- Volley: RISING_STREAKS GREEN on the loose + OVAL_RING WHITE at the target, STREAK_RAIN WHITE per wave.
- Only Counts as One: LIGHTNING WHITE line, TENDRIL_STAR GOLD on the target, SFX_COMBAT_TRIPLE_DAMAGE.

## PNG sprite effects (hacks/content/skill_fx.json)

Real art beats recolored stock shapes: Sam's plants were stock sparkles and "did not look like plants".
- Sheets: PNG strips in `hacks/content/fx/`, one frame per cell, at most 15 opaque colors, no partial alpha.
- Manifest asset: key, id (palette id 0..31), sheet, width, height, frameTicks, anchor ([x, y] = the world point, usually the feet), optional `palette`.
- `palette`: a strip PNG shared by several sheets. Same id + same strip = one OBJ palette slot for all of them. Use it for any hero kit (Sam: `fx/sam_palette.png`, id 1).
- Effects: append with the next consecutive id, layers of `asset` or stock `shape` + fixed `color`. C gets `HACKLIB_EFFECT_<KEY>` from build/hack_include/skill_fx_ids.h.
- Play: `hackLib_playEffect(HACKLIB_EFFECT_X, actor, &pos, NULL)` (lib/skill_fx.h). With pos it stays at pos.
- Previews: `uv run python hacks/tools/skill_fx.py` writes build/skill_fx_preview/index.html (GIFs). The Debug Lab effect preview plays any effect in game.
- Procedural art: `uv run python hacks/tools/sam_fx_art.py` redraws Sam's sheets (Canvas with leaf, ellipse, curve, outline helpers). Copy it for other heroes or replace a PNG with hand art in the same palette.
- A burst ends as it ENTERS its last frame. skill_fx.py appends a hidden copy of the last frame so every drawn frame shows (an n-frame sheet lasts n * frameTicks). Hand-built SpriteAnimations still lose their last frame.
- Looping a persistent prop (plant, pot): end the sheet on its first pose and respawn 2 ticks before it ends, so the overlap shows two identical frames (Sam: 5 frames x 4 ticks, respawn every 18). A gap makes it flash. When the prop dies, wait for the current loop to end before its death anim (Sam `SAM_PLANT_WILTING`), else both draw at once.

## Sam map (page 2, PNG sprites)

| Skill / event | Visual |
|---|---|
| Plant Taters / Bramble Hedge cast | sam_sow at the spot (seed lands, mound heaves, shoot), herb sfx |
| Tater plant | sam_tater_sprout -> sam_tater_leafy -> sam_tater_ripe (flowers, potatoes in the soil, glint), swaying loops |
| Ripening | SPARKLE_RING GOLD at the plant |
| Tater pelt hit / Po-tay-toes hit | sam_spud_hit on each foe (potato bonks the head and bounces) |
| Harvest / Po-tay-toes throw | sam_harvest at the plant (potato pops out of the mound), herb sfx |
| Bramble plant | sam_bramble_sprout -> sam_bramble_bush -> sam_bramble_hedge (48 px, red berries) |
| Bramble pulse hit | sam_thorn_jab on each foe |
| Wilt | sam_wilt (droops, browns, crumbles), after the last plant loop |
| Second Breakfast | sam_stew_pot loop (fire, bubbles, steam), item sfx, SMOKE_RISE WHITE when it ends |
| Frying Pan Parry | timed DIAMOND_RING GHOST + sam_pan_clang, pan clang per blocked hit |
| Bonk! | sam_pan_bonk (pan + STAR_BLOOM GOLD + WIDE_RING WHITE) + SHOCKWAVE GHOST |
| Elven Rope | sam_rope_knot along the throw, sam_rope_lasso cinching the foe |
| I Can Carry You | RISING_STREAKS GOLD (unchanged) |

## Gimli map (page 2 "The Tally", v31)
Colors: Stone = WHITE / GHOST, Fury = BLOOD / EMBER / NIGHT, Tally = GOLD. All stock shapes, no PNG sheets yet.
- Baruk Khazad!: WIDE_RING white behind + STAR_BLOOM gold front (paced burst), SPARK white on each stunned foe, guard = fxTimed DIAMOND_RING ghost for the guard time.
- Cleave the Weak: execute = TENDRIL_STAR blood on the foe, plain hit = SPARK ember.
- Nobody Tosses a Dwarf: SMOKE_SMALL at the take-off point (fxAt), SHOCKWAVE_ALT gold on landing.
- Hold the Deep: fxTimed SPARKLE_RING white for 3s + GROUND_POOL ghost. Release: SHOCKWAVE ghost + SHOCKWAVE_ALT ember.
- Book of Grudges: TENDRIL_BURST blood (paced) + OVAL_RING ember on the marked foe.
- Still Counting: fxTimed RISING_STREAKS blood for the duration + TENDRIL_STAR night. End burst: SHOCKWAVE night + RISING_STREAKS blood.
- Empowered cast (charge bar full): STAR_BLOOM gold + fxTimed SPARKLE_RING gold 5s. Charged: SFX_SKILL_TREE_BUY_ACTIVE.
- Passives: Not Without Me SPARK white on Gimli, Orc-Count SPARK gold, Durin's RISING_STREAKS gold on save and STAR_BLOOM gold at Rage start, Forty-Two SHOCKWAVE night + TENDRIL_STAR gold, Friendly Wager fxTimed SPARKLE_RING gold 8s, Grudgebearer/Grudges SPARK blood per grudge hit, Blood on the Axe SPARK blood per extension.
Trap: the stock Rage aura is a timed fx of type 8 parented to Gimli (`as.gatePanel.tileSet == 8`, tick fx_tickTimedAttachment). Do not parent your own type-8 timed fx to Gimli or Blood on the Axe will extend it too (bursts on foes are fine).
