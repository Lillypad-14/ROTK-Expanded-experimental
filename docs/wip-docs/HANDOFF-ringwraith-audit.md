# Ringwraith audit handoff, updated 2026-10-21

Status: the mechanics audit and the first shared ability-support pass are implemented and regression-tested.
The expanded engine pass adds terrain-safe landing, cast receipts/results, hero-owned skill tables and combat hooks, reusable ability helpers, build validation and a Combat Lab.
Visual/native-AI playtesting remains open.
For reusable APIs, start with `docs/ability-support.md`.
This note records the current session and supplements `HANDOFF-ringwraith-page2-and-gear.md`.
Code is the source of truth.

## User decisions

- Ringwraith is a single-weapon hero. Do not give it Aragorn's dual wield.
- The unmodified summon cap is 2 thralls.
- Thralls must support viable summon builds, keep attacking and acquire new targets.
- Morgul Blade should have visible sword glints as well as its palette glow.
- Audit every Ringwraith active, passive and gear stat/effect.
- Co-op expansion-state transport is explicitly deferred by the user.
- The user requested this handoff for Claude and future implementations.

## Delivered ROM and current source

Latest build: `~/Desktop/lotr/ROTK-Mod-Ringwraith-v11.gba`.
Its save was copied from v10 without overwriting existing numbered saves.
The v8, v9 and v10 ROMs and saves were preserved.
v11 contains the complete current mechanics fixes, shared engine support and the hero/ability framework.
At delivery, v11, `build/rotk_hack.gba` and `build/hack.elf` match.
ROM SHA-256: `4305c9db7681b8b1cc36da9a0cf3ab63f902d2f9453ed230e50f9661ea274186`.

The v9 ROM predates the broader mechanics audit.
v10 contains the broader mechanics fixes and first three shared helpers, but predates the expanded framework.
Do not assume a numbered ROM and build/hack.elf remain identical after another build.
Most Ringwraith and skill-expansion source was already untracked at session start.
Do not discard it or commit ROMs/assets.

## Implemented fixes

- `skill_expansion_values.c` replaces `skill_active_getLeveledValue` with bounded curve reads.
  Temporary active levels can exceed 5, but value curves have exactly 5 entries.
  Keep the counters intact so removing buffs remains balanced.
  The existing equipment skill-level bonus still repeats the fifth increment at the cap, matching stock behavior.
  The replacement has 4 arguments and uses `hookReplaceR3`, not `hookReplace`.
- `debug_cheats.c` Max Level includes the extra point for every skipped zero-based level >= 20, matching the expansion's normal level-up hook.
- `ringwraith_thralls.c` installs a per-thrall callback, not a global change to Aragorn's ghosts.
  It recovers from the stock ghost's post-attack death path and retargets dead/unhittable targets.
  It delegates to `actor_npc_tickPassive` for the existing lifetime and removal bookkeeping.
  Lifetime <= 1 and explicit despawning bypass recovery, preserving Command of the Nine consumption.
- Thrall base lifetime is 600 frames, with 180 more per Morgul Sorcery level.
  Damage is 8 + 4 * the shard's Morgul Blade level, before gear, capped at 255.
  Base cap remains 2. Morgul Sorcery adds one at levels 2 and 4, Host of Angmar adds one per level, Banner adds one.
  Summoning no longer requires a second living enemy to already be present.
  The Morgul Sorcery tooltip was updated from +2s to +3s per level.
- Morgul Blade plants its shard before damage, so killing with the cast can raise the victim.
- Morgul Blade has a 72-frame pale-green blade glow and repeated white `FX_SHAPE_SPARK` glints.
  The glints follow the animated sword hitbox and horizontal flip, with a fallback position.
  No new bitmap assets were generated or committed.
- Black Breath now heals from actual HP removed on successful hits, excluding misses and overkill.
- Drain healing retains fractional HP across hits, making small Life Drain values functional.
  Shadow Hunger, Drain Healing gear and Black Sacrifice apply to melee drains and Black Breath through `healDrain`.
- No Living Man's kill heal now receives the advertised Drain Healing equipment bonus.
- Will of Sauron can raise kills from new waves even when those enemies do not already have a Dread mark.
  Player-hit kills consume their shard mark after raising, avoiding a second summon in `updateMarks`.
- Reused actors with a different unit type invalidate marks without spawning a thrall.
- Adding Dread ignores dead/unhittable foes, preventing corpse terror/Spirit gains.
- Morgul-Knife and Unseen Black Dart shards now summon on killing melee hits.
- Ninth Ring now counts killing melee hits toward its five-hit proc.
- Fell Beast and Spurs preserve a longer existing terror stun.
- Wraith Form applies its advertised 20% weakness to fire-tagged enemy melee attacks.
- Shared `hacks/lib/combat_result.h`, `status.h` and `summon.h` provide reusable damage snapshots, stun extension and persistent ghost behavior.
  `skillExpansion_applyRangedHitResult` exposes actual primary-hit results to abilities, including Black Breath.
  Results are captured before secondary effects and remain stack-local for nested hits.
  Both Ringwraith and elite ghost iteration exclude lifetime <= 1, preventing reuse of consumed summons before native cleanup.
  Persistent ticking is opt-in, so stock Aragorn ghosts retain their stock attack lifecycle.
- `ringwraith_kitOnHit` now has a sixth argument, `damageDealt`.
  Both combat-wrapper call sites in `skill_expansion_elite.c` pass the victim's actual HP loss.
  Update any future callers too.

An attempted dual-wield change was removed after the user clarified the design.
`wraithGear_getEquipSlot` still uses the existing single-weapon behavior.

## Reverse-engineering findings

These are inspection findings, not newly byte-matched decompilation units.
No new `src/c` implementations or split entries were added during this session.

| Address | Finding |
| --- | --- |
| `0x08035864` | Summon constructor: kind, position pointer, owner, target, lifetime. Fifth argument is on the stack. |
| `0x08035940` | NPC setup called by that constructor. |
| `0x08035bac` | `actor_npc_tickPassive`, including lifetime decrement and ghost removal. |
| `0x08009e44` | `actor_runBehaviorState`; behavior 0x20 calls `0x0800512c`. |
| `0x0800512c` | Ghost combat behavior. Action 3 + animation end enters 0x3d. That leads to action 4 and behavior 1, ending a stock ghost after one attack. |
| `0x08016354` | Ghost collision callback. Uses the owner as attacker for variant 0x1a and reads damage at actor byte 0x95. |
| `0x08010380` | `combat_applyPveRangedHit`, called by the ghost callback. Thrall hits therefore enter the player's ranged-hit wrappers. |
| `0x08043764` | Stock leveled-value lookup reads past `perLevel[5]` when temporary levels exceed the cap. |
| `0x08044c90` | Gandalf Sword of Power callback. This changes the player action to 0x46 and sets cooldown bit 0. It is not a drop-in sword-glint routine for Ringwraith. |
| `0x080430e8` | `skill_active_cast` deducts Spirit before its special immediate Herbal Healing callback path. Black Breath is row 5 and still pays its Spirit cost. |

Ghost payload layout already lives in `skill_expansion_elite.h`: target pointer, owner pointer, u16 lifetime, and damage byte at payload offset 0x11.
The owner count is `owner->as.combat.comboState[0]`.
The stock NPC removal path decrements it, so do not also decrement it in a wrapper.

Ghidra was consulted first using the local GhidrAssist MCP client.
The user authorized starting the read-only server with `start-ghidra-mcp.cmd`.
Server URL is `http://127.0.0.1:8080/mcp` and the client is `%LOCALAPPDATA%/rotk-tools/gh.py`.
Some functions, including the summon constructor and ghost behavior/collision routines, were not defined in the loaded Ghidra project.
Only after Ghidra returned "Function not found" was objdump used for those ranges, as AGENTS.md permits.
Several existing Ghidra pseudocode returns incorrectly appear as unrecovered indirect calls.
Use its disassembly to check the actual Thumb return instructions before trusting that output.
No symbols/comments were changed in Ghidra.
Raw inspection outputs are the neighboring `ghidra-*.txt` files and `inspect_thralls.py` can refresh them.

## Verification tools and limits

Run from the repository root:

```text
python local/wip-docs/verify_skill_bugfixes.py
python local/wip-docs/verify_thralls.py
uv run --with unicorn python local/wip-docs/audit_ringwraith.py
```

`verify_skill_bugfixes.py` compiles the actual patch C with host-side stubs.
It checks all u8 level counts, signed curves, equipment level bonuses, both player indices, every old level and point count, saturation, repeated Max Level ticks, and builds with/without skill expansion.

`verify_thralls.py` compiles the actual thrall callback with a captured stock-tick boundary.
It checks repeated attacks, retargeting, idle summons, expiry, and consumed summons.
It does not execute the native NPC AI or collision/render system.

`audit_ringwraith.py` executes the compiled Thumb code from `build/rotk_hack.gba` using Unicorn.
It extracts struct offsets with the actual ARM compiler and symbol addresses from the matching build.
It checks the actual equipment stat application/removal routines in the ROM.
World iteration, summon allocation, ranged hit resolution, resource-clamping backends, sound and FX allocation are deterministic stubs.
It is a mechanics regression harness, not a full GBA emulator or a visual playtest.
The harness's `cast` invokes the actual callback directly after setting `castingActiveSkill`.
It does not simulate cast animation timing or the full controller loop.

Results so far:

- All 11 active callbacks at levels 1 and 5 pass basic effect checks and cast-eligibility checks with valid setup.
- All 99 wraith base items pass native equip-slot, stat grant, and stat removal checks.
- All 18 passive slots pass targeted main-effect checks.
- All 19 unique powers pass targeted main-effect checks.
- The five additional healing/new-wave regression cases reproduced against the prior build and pass with the fixes above.
- `make hack` passes after the current fixes.
- All 252 appended prefix/suffix combinations across seven item types pass native stat, custom effect and removal checks.
- Killing-shard, Ninth Ring kill, stun preservation and fire-melee regression checks pass.
- First and last save slots retain hero, gear, expanded purchases, cycle flags and quick-select rows above 7.
  The save harness stubs native inventory/stat reconstruction at 0x0802ac24 because native stat application/removal is checked separately.
- Primary damage result checks pass for hits, misses, overkill, already-dead targets and secondary-effect mutation.
- Final integrated run: all 16 ARM mechanics check groups pass against the v10 build.
- `python hacks/tools/test_ability_support.py` compiles the actual shared headers against real actor declarations.
  It checks damage credit, stun rules and persistent ghost behavior with native ticking/FX stubbed.
- v10 used 1924 / 2048 bytes. The later engine pass uses a larger reservation and transient runtime/projectile state, without save-format changes.

A headless mGBA attempt did not produce a usable debug session, so these new changes have no completed visual/full-emulator playtest yet.
Only that attempt's own helper processes were stopped.
Do not terminate all mGBA/GDB processes in future sessions.

## Engine and skill framework pass, 2026-10-21

Read `docs/ability-support.md` for the stable API contracts and limitations.
The Ringwraith definition now owns both active pages and both passive pages.
`HeroKits` is the central custom-hero registry.
The old content tables remain stock-hero fallbacks.
Do not add future custom-hero branches to the common hit wrappers.

The shared HP hook moved from `ringwraith.asm` to `skill_expansion.asm`.
Its native thunk is now `skillExpansion_original_player_addHp`, not `ringwraith_original_player_addHp`.
Ringwraith keeps full resource interception through `kit.onHpChange`.
Other heroes can use incomingDamage and leave onHpChange empty.

Native addresses added to config/prototypes:

- 0x08034770: terrain_getCollisionAt(pixelX, pixelY), returns a collision byte.
- 0x0803f2a8: terrain_getWorldSize(Vector2Int *), pixel dimensions.
- 0x03002068: g_TerrainCollisionCells, byte-cell data.
- 0x0300206c: g_TerrainCollisionBlocks, 32-pixel block-index map.
- 0x08016354: actor_npcGhost_trigger(self, other), now wrapped for source attribution on tracked ghosts.
- 0x080337c8: actor_projectile_create(owner, direction, direction32), returns Actor* despite Ghidra's inferred void return.
  Ghidra disassembly at 0x08033d04 moves r7 to r0 before returning, confirming that return value.

Ghidra was queried first for all newly inspected native functions.
Undefined terrain helpers were examined with objdump only after Function not found.
No native decompilation units were byte-matched or added to split.cfg.
The native collision query returns zero outside map bounds, so callers must not treat zero alone as a valid floor.
The shared landing helper checks bounds first and rejects nonzero low collision categories conservatively.

Cast results and refund rules:

- A cast receipt retains the actual paid Spirit, not a later table cost.
- A free debug cast refunds zero.
- Failed completion or an interruption refunds once, clamped to the maximum.
- Loadout changes discard receipts rather than credit another save/hero.
- Morgul Blade and Shadowstep now use result-returning resolvers.
- A failure result does not roll back world effects already applied by an ability.

Damage source contexts are synchronous and stack-local, with a single current-context pointer restored around nested calls.
Known secondary bursts participate in a depth limit of eight.
Source metadata does not provide critical-hit or elemental flags for every native damage path.
The generic projectile helper has eight tracking slots shared between players and preserves stock flight/art/lifetime.
Its impact callback supplies custom damage instead of native weapon impact effects.
No existing Ringwraith ability was changed to shoot these new generic projectiles.

`make hack` now validates linked content before writing the ROM.
Compiler-derived layout data is emitted by skill_expansion_validation.c.
The validator checks both custom hero pages and appended stock definitions.
Local `verify_ability_content.py` mutates linked content to prove missing icons/callbacks, bad rows, bad curves and invalid passive IDs are rejected.

RAM reservation is now 4096 bytes starting at 0x0203f000.
This reduces native heap space by 2048 bytes compared with v10.
The native heap initializer and a native zeroed allocation pass ARM regression checks with the new boundary.
This does not prove every asset-heavy mission has sufficient peak heap headroom.
No save-format or persisted option changes were made.

Combat Lab is an opt-in Mod Options page under the debug_cheats patch.
It spawns fixtures in a cleared existing bonus-map room, not a newly authored mission.
Native spawning is stubbed in the mechanics tests, so fixture art/AI still needs emulator verification.
Lab commands do not run in co-op.
Pickup and interaction text takes priority over feedback and lab telemetry through the player_tickHud wrapper.
The wrapper replays the exact native 8-byte prologue verified using Ghidra disassembly.

## Remaining work and verification limits

- Complete emulator testing of terrain edge cases, sword glints, HUD message layout, native ghost attacks, test fixtures and projectile flight.
- Destination safety is terrain-only. Dynamic actor overlap and complete dash/knockback path checks are separate work.
- New native hero IDs still need animation/portrait/equipment registration outside the skill definition.
- Native projectiles outside the new helper can lack an originating ability row.
- Untracked native proc paths do not automatically participate in the typed secondary-depth limit.
- A live hero switch must perform the normal stat/loadout rebuild, not only assign Player.heroId.
- Co-op expansion-state transport remains deferred.

## Build/environment notes

Use Git Bash and put `%LOCALAPPDATA%/rotk-tools/arm-gcc/bin`, `rotk-tools/armips` and the WinGet Links directory on PATH, then `make hack`.
The current `MODCODE_BSS` grant is 4096 bytes.
Check `arm-none-eabi-size build/hack.elf` before adding per-player or per-actor state.
Replacing the pending-damage s16 with a fractional-healing u16 preserved state size.
`git diff --check` can flag existing CRLF lines as whitespace. `git -c core.whitespace=cr-at-eol diff --check` handles this without changing Git configuration.
Never copy generated art or the ROM into a commit.

## Final v11 checks

- `make hack`: PASS, including 16 linked active definitions validated before ROM patching.
- `uv run --with unicorn python local/wip-docs/audit_ringwraith.py`: all 24 check groups PASS.
- `python hacks/tools/test_ability_support.py`: PASS.
- `python local/wip-docs/verify_ability_content.py`: all five corrupted-content mutations rejected.
- `python local/wip-docs/verify_skill_bugfixes.py`: PASS in both build modes.
- `python local/wip-docs/verify_thralls.py`: PASS.
- `git -c core.whitespace=cr-at-eol diff --check`: PASS.
- Linked sections: 123686 text/rodata bytes, 0 data bytes, 2184 BSS bytes out of 4096 reserved.

The new mechanics checks include shared area damage and target selection, ghost caps/consumption, projectile tracking and single-hit removal, generic incoming-damage and kill hooks, cast receipts/refunds, blocked landings, source nesting/depth limits, lifecycle cleanup, native heap allocation, lab command gating, pickup-text priority and large healing without signed overflow.
Native world iteration, hit resolution, FX and actor factories remain stubbed where described above.
Terrain lookup and heap initialization/allocation execute native ROM code against deterministic synthetic inputs.
All native AI/rendering claims still need an emulator pass.
No commits were created.
