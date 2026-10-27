# Engine changes

What ROTK Expanded changes in the game engine, relative to the upstream
[lotr-gba-re/rotk](https://github.com/lotr-gba-re/rotk) base (commit `b4f8637`). The upstream hook framework is
described in [hacks/README.md](../hacks/README.md). This page covers what Expanded built with it.

All of these changes live in the hack build (`make hack`). The byte-matched decompilation build (`make verify`) is
unaffected and still reproduces the original ROM exactly.

## How the engine is changed

Expanded uses three kinds of change, in order of preference:

1. **Whole-function replacement** (`hookReplace`, `hookReplaceR3`). The stock function is redirected to new C.
   The original body stays callable as `<patch>_original_<name>`, so most replacements wrap the stock behavior
   instead of rewriting it. There are about 95 of these across the patches.
2. **Small in-place edits** (`.org`). A few instructions are rewritten at a fixed site, always with a
   `; was:` comment giving the stock instructions. About 20 of these exist.
3. **Table repoints.** A literal pool word that points at a stock table is pointed at a larger copy. Every repoint
   checks with `readu32` that the site still holds the stock address, so a wrong address fails the build instead of
   corrupting the ROM.

Code and data that do not fit in the stock ROM go in the expansion area past the end of the cartridge image.

## Memory layout

`hacks/layout.cfg` was widened for the larger mod:

| Region | Upstream | Expanded |
| --- | --- | --- |
| `HOOK` (asm hook handlers) | `0x09000000-0x090001ff` | `0x09000000-0x09000fff` |
| `MODCODE` (mod C code and data) | from `0x09000200` | from `0x09001000` |
| `MODCODE_BSS` (mod RAM) | `0x0203fd00-0x0203ffff` | `0x0203f5a0-0x0203ffff` |

The `mod_ram` patch now replaces the game's allocator (`memory_init`, `malloc`, `zalloc`, `free`, `realloc`), the
actor allocators (`actor_allocMain`, `actor_allocOverlay`) and `sprite_free`. This adds:

- a heap that starts after the real end of the mod's BSS instead of a fixed address
- allocation diagnostics (peak use, failures, and `Room:`, the bytes between the highest allocation and the heap
  end), shown in the debug pages
- two guard words at the start of the mod BSS (`modRam_bssGuard`), so a heap overrun into mod RAM shows `GUARD!`
- an actor lifecycle notification, which gives every actor a generation number so mod code can hold actor
  references that safely go stale (see `hacks/lib/actor_ref.h`)
- a shared graphics scratch buffer

The `MODCODE_BSS` budget is the lowest address mod RAM may reach. The heap ends where a build's BSS actually
starts, so a larger budget only costs heap when a build uses it. The main build starts its BSS at `0x0203f5e0`.

## Save format

The save is extended without breaking upstream saves:

- **Global mod options** gained a byte for Debug cheats and 17 bytes for Twists, carved from the reserved space.
- **Region 5:** one 32-byte skill-expansion record per save slot on every save page.
- **Region 6:** one 144-byte versioned record per slot that stores page 2 purchases, extra active rows, passives
  and quick-select rows by skill id (tag `0x73`, version 1). Records stay valid when skills are reordered.
- `scene_saveLoadMenu_loadSlot` and `loadPeerSlot` are wrapped so a slot's expanded state loads with it, including
  co-op peers. After the stock co-op slot exchange, each console sends its player's page 2 ranks, cycle flags and
  quick-select rows in link message `0x0b` (`skill_expansion_coop.h`) before the peer's slot loads.

`_Static_assert`s in `save_layout.h` fail the build if the regions overrun the save chip.

## The skill system

The stock skill system has six active rows and nine passives per hero, with no room for more. `skill_expansion`
replaces all of the skill bookkeeping:

- **Active skills:** add, apply, purchase, temporary levels, `hasLevels`, `getLeveledValue`, `canCast`, `cast` and both
  quick-cast input modes are replaced to work on 12 rows plus extra rows.
- **Passive skills:** all nine stock passive-level functions (add, apply, remove, temporary levels, apply-all and
  per-level stats) are replaced to work on 18 passives.
- **Hero loadout:** `player_initHeroLoadout`, `resetActiveSkills` and `resetActiveSkillControls`.
- **Skill tree screen:** D-pad, A, exit, both tile drawers, the passive grid, the description box, required levels,
  quick-select markers and assignment are replaced to give the two-page tree with L/R flipping.
- **HUD:** `player_tickHud` and `player_updateHudSkillPalette`, for page 2 icons, hero HUD bars and pickup text.
- **Input:** `input_update`, for hold-R form swaps and other held-button mechanics.

In-place edits:

- `player_awardXp`: from level 21 on, a level-up grants two skill points (the attribute points are unchanged).
- `actor_player_tick`: page 2 actives use their base row's casting pose instead of reading past the pose table.
- `scene_gameplay_exit` and `actor_player_create`: Frodo's corruption (Burden) is no longer reset when changing zones.

Each hero plugs in through a **hero kit** (`SkillExpansionHeroKit` in `skill_expansion.h`): its rows, icons,
required levels, descriptions, passives, and optional callbacks:

| Callback | Called when |
| --- | --- |
| `canCast` | A row has an extra cast condition. |
| `reset`, `cleanup`, `tick` | Transient state is reset, the actor is torn down, or every frame. |
| `meleeDamage`, `rangedDamage` | Outgoing damage is computed. Returns the new damage. |
| `onHit`, `onKill` | A hit or kill by this hero resolves. |
| `onCombatEvent`, `onIncomingCombatEvent` | Any outgoing or incoming resolution, including misses. |
| `incomingDamage` | An enemy attack is about to land. Returns the new damage. |
| `onHpChange` | HP changes through `player_addHp`. |
| `refundSpirit` | A cast refunds Spirit. |

Page 2 heroes keep their transient state in one shared per-player block, `skillExpansion_heroState`. Each hero
reaches it through `skillExpansion_heroStateClaim`, which zeroes the block when its owning hero changes and counts
any access for a hero the player is not in `skillExpansion_heroStateMisuse`. Hooks that run for every player fetch
the block only after their hero check.

## Combat system

Stock combat has no notion of who or what dealt damage, no hit or kill events, and no shared way to stun, mark or
summon. Expanded adds a combat layer around the five stock resolution functions.

### Replaced combat functions

| Stock function | What the wrapper adds |
| --- | --- |
| `combat_computePveDamage` | Records the native crit result and the attacker's fire/poison elements, then applies hero bonuses in a fixed order: Aragorn ambush crits and mark bonus, Frodo, Smeagol, Sam, Gimli, Gandalf, the hero kit's `meleeDamage`, then Twists and debug scaling. |
| `combat_resolvePveAttack` | Melee hit resolution. Sets the damage source, captures the HP before and after, builds a combat event and dispatches it. Handles Aragorn's ambush, patience and stun extensions. |
| `combat_applyPveRangedHit` | The same for projectiles and ghost hits, with ranged bonuses including Legolas and the kit's `rangedDamage`. |
| `combat_resolveEnemyAttackOnPlayer` | Enemy attacks on a player. Tags the source with the enemy's fire/poison flags, observes the real damage taken and dispatches an incoming event. Strider's Riposte triggers on a miss. |
| `combat_handleEnemyAttackOnPlayerStatusArmorSfx` | Status, armor and hit-sound step of incoming hits. Records which elemental statuses actually applied. |
| `player_addHp` | Every HP change. Heroes observe damage and healing, and nested changes are attributed correctly. |
| `player_addSpirit` | Spirit gains, so the Ringwraith gains Spirit only from its kills. |
| `actor_player_tick` | Per-frame hero timers, regeneration, Twists and debug ticks. Runs before the stock tick. |

Ghost summons get their trigger callback swapped for a tracked version, so ghost hits count as summon damage and
can feed hero meters.

### Damage results

`HackLibDamageResult` (`hacks/lib/combat_result.h`) replaces the stock functions' plain `bool` hit flag:

| Field | Meaning |
| --- | --- |
| `hit` | The attack connected. |
| `killed` | The victim went from above 0 HP to 0 or below on this hit. |
| `damageDealt` | HP actually removed, clamped at the victim's HP. |

### Damage source context

`HackLibDamageSource` (`hacks/lib/combat_source.h`) records what caused the damage currently being resolved. It is
set around each resolution with `hackLib_swapDamageSource` and read with `hackLib_currentDamageSource`.

| Field | Meaning |
| --- | --- |
| `origin` | The actor that caused the damage. |
| `kind` | `WEAPON`, `PROJECTILE`, `ABILITY`, `SUMMON`, `PERIODIC` or `SECONDARY` (`HackLibDamageKind`). |
| `abilityRow` | The skill row for ability damage, or `HACKLIB_ABILITY_NONE`. |
| `depth` | How deeply this damage is nested inside other damage. |
| `elements` | `HACKLIB_ELEMENT_FIRE` and `HACKLIB_ELEMENT_POISON` bits. |
| `elementsKnown` | Separates "not elemental" from "not specified". |

Damage caused by other damage (a flame burst from a hit, a chain kill) nests the source. Nesting stops at
`HACKLIB_MAX_DAMAGE_DEPTH` (8), so chains of effects cannot recurse forever.

Abilities deal damage through `skillExpansion_dealDamage` and `skillExpansion_dealElementalDamage`, which run the
full ranged-hit path with an `ABILITY` source.

### Combat events

`HackLibCombatEvent` (`hacks/lib/combat_event.h`) is captured right after each native resolution:

| Field | Meaning |
| --- | --- |
| `attacker`, `victim` | The two actors. Valid only during dispatch. |
| `source` | A copy of the damage source. |
| `result` | The damage result. |
| `melee` | Melee or ranged. |
| `critical`, `criticalKnown` | The native crit roll, when it was observed. |
| `appliedStatusElements`, `appliedStatusElementsKnown` | Elemental statuses that actually applied. |

`hackLib_dispatchCombatEvent` calls the `kill` handler first, then `hit`, then `resolved`, and `resolved` also
receives misses. Each resolution dispatches exactly once. Debug Lab and Twists record every event too.

### Shared combat helpers

| Header | Provides |
| --- | --- |
| `status.h` | `hackLib_applyStun`, which respects stun immunity, never shortens an existing stun and plays the stock stun effect. |
| `timed_effect.h` | A pool of 12 timed effects on actors with `apply`, `pulse` and `remove` callbacks and refresh, stack or replace rules. Used for marks, curses, slows and zones. |
| `summon.h` | Views and helpers for stock ghost summons: owned-ghost iteration and persistent ghosts that retarget. |
| `targeting.h` | Target queries by radius, facing cone and filter, nearest-target search and position queries over live actors. |
| `actor_ref.h` | Generation-checked actor references that go invalid when an actor is freed and its slot reused. |
| `movement.h` | Landing checks for dashes and teleports, so moves cannot drop a hero inside a wall. |
| `timer.h` | Saturating frame timers. |
| `cast_timeline.h` | Frame timelines for casts: visual, release and finish events anchored on the caster, target or a point. |

### New names in the shared headers

Decompiling the combat code put these into `include/`:

- `actor_stun`, `actor_checkDeath`, `actor_applyPersuasion` and `ACTOR_COMBAT_FLAG_PERSUADED`
- `actor_npcGhost_trigger`, `actor_npc_initBehavior`, `actor_projectile_create`
- `fx_statusEffect_tick` and the stun sound cooldown functions
- `skill_active_getSpiritCost` and the Frodo knife and snare, and Gimli axe-throw and Earth Shatter, tick functions
- `PLAYERHUD_FLAG_SKILL_PANEL_DISABLED`, `PLAYERHUD_FLAG_BOSS_BAR`, `PLAYERHUD_FLAG_COMPASS_COUNT` and
  `PLAYERHUD_FLAG_HERB_COUNT`

## New heroes

The stock game has eight heroes and many fixed-size, per-hero tables and `heroId` switches. The `ringwraith` patch
makes room for hero ids 8 (Ringwraith) and 9 (Saruman):

- **Hero registry.** `hacks/content/heroes.json` maps each hero to a stock hero for model, equipment class and skill
  tree art. `hacks/tools/hero_registry.py` generates the tables and validates them.
- **Table repoints.** 27 literal pools for the move, melee and cast animation banks are repointed to larger tables,
  along with the portrait, skill tree, loot, start mission and camera tables.
- **Switch bounds.** `cmp r0, #7` bounds in start items and backpack art, the palette choice and the skill tree
  title are widened.
- **Equipment aliases.** Four item-class checks route through the registry, so a new hero can use another hero's
  equipment.
- **Replacements:** `player_initHeroStats`, `actor_player_updateHandSprites` (a body with no hand parts skips them),
  and the save slot drawer and refresher, so the new heroes show on the save and hero select screens.
- **Wraith gear:** `loot_generateItem`, `item_affix_formatName` and `item_getEquipSlot` are replaced, and seven
  `ItemTypeInfos` rows point at extended tables. The stock rows are copied from the ROM and the wraith rows are
  appended after them.

## Scene start and respawn

`scene_gameplay_enter` calls `scene_gameplay_healAllPlayers` (0x08032434) for the start modes that heal everyone
(save load, bonus map and co-op level select launches). The `ringwraith` patch hooks it, because two things need
fixing around that call:

- **Full heal.** The wraith's `player_addHp` refuses every gain except its own drains, so the stock heal left it at
  its old health. The hook raises a `fullHeal` flag in the kit around the call, which lets that one heal through.
  Spirit stays refused, since the wraith starts missions at a quarter of its Spirit.
- **Burden.** `skill_expansion.asm` removes the stock `corruption = 0` writes at zone exit and player spawn so
  Frodo's Burden carries between zones. That also carried it across save loads, so the hook clears the meter for both
  players and re-applies the tier flags with `player_addCorruption(p, 0)`.

Anything that changes `currentSpirit` without `player_addSpirit` must set `PLAYERHUD_FLAG_SPIRIT_DIRTY` on
`g_PlayerHuds[p]`, or the HUD keeps drawing the old value. The wraith's start clamp and its refused Spirit gains do.

## Effect actors that run their own logic

An imported sprite effect is an ordinary actor, so a skill can replace its `tickCallback` and keep per-effect state in
the actor's `as.combat` fields (`venomDuration` as a frame counter, `storedDamage`, `playerIndex`, `comboState`).
Fell Beast does this: the callback moves the beast, loops its animation, and strikes foes it crosses, with no static
RAM. The mod's RAM budget (`MODCODE_BSS`) has no spare bytes, so new per-effect state belongs in the actor.
If no effect actor can be allocated the cast falls back to an instant strike, because effects must stay cosmetic.

## Skill icon colors

Page 2 icons can carry their own colors. The stock frame uses palette indices 0..7, so each icon gets 8
colors in indices 8..15.

- The tree loads an icon's color group into BG palette bank 9 + group (banks 9..15 are free) and repoints the tile.
- `skillExpansion_hudUpdateSkillPalette` loads the selected icon's colors into HUD bank 1 and halves them when the
  skill can't be cast. It uses the stock colors while the unspent-points graphic shows, since that graphic shares
  the bank.
- Icons come from `skillExpansion_iconColors`, a lookup over each hero's generated `<Prefix>Colors[]` table.
  `hacks/tools/skill_icon_art.py` builds the tables.

## HUD

`hacks/lib/hud_meter.c` draws hero bars in the stock HP/Spirit style, in the gap between the two bars:

- plain bars with threshold notches
- segmented bars and continuous meters, including Saruman's empowerment meter

A draw is skipped when its inputs and the meter's VRAM match the previous draw, which keeps the per-frame cost
near zero while still repairing the bar after a HUD reload.

The meter owns BG char tiles `0x62..0x6a`. Tiles `0x4d..0x55` look unused but the compass draws its herb and item
counts there.

## Other patches

| Patch | What it changes |
| --- | --- |
| `twists` | Playthrough modifiers in the mod options (damage dealt and taken, life drain, lucky strikes, regen, kill rewards, XP gain, Omens, Ring's Whisper, Fellowship and more). Wraps `player_awardXp` and the combat hooks above. |
| `debug_cheats` | A Debug options page (max level, god mode, infinite Spirit, one-hit kills and more), the Debug Lab combat readout, Ability Bench and delivery preview. |
| `unlock_all` | An Unlocks options page, for example every hero path in Chapter Select. |
| `test_arena` | A boot-to-arena build for testing abilities (`make test-arena`). Stubs mission spawning and fixes the RNG seed. Also runs the self-driving hero sweep and the busy fight benchmark (see [test-arena.md](test-arena.md)). |
| `fast_waitstates` | Faster ROM wait states at startup. |
| `fast_obj_tiles` | Faster OBJ tile bitmap search within the original ARM/IWRAM buffer. See [measurements and validation](performance.md). |
| `fast_oam` | Sprite OAM submission without a stack frame. See [measurements and validation](performance.md). |
| `fast_sprite` (optional) | Experimental frame composition with unused dimension lookups skipped. Disabled by default after mixed timing results. See [performance](performance.md#experimental-frame-composer). |
| `item_descriptions`, `text_overrides` | Text for new items and skills, and overrides of stock strings. |
| `mod_options`, `save_slot_pages`, `mod_startup` | Extended for the new pages, heroes and startup hooks. |

## Where to read more

- [Ability support](ability-support.md): the shared ability API in detail.
- [Combat validation](combat-validation.md) and [hero move checks](hero-move-checks.md): what has been tested.
- [Hacking recipes](wip-docs/hacking-recipes.md): reverse-engineering recipes and traps found along the way.
- [Source inventory](source-inventory.md#mod-patch-inventory): every patch and tool.
