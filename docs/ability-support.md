# Shared ability support

Hack helpers live in `hacks/lib/` and use the `hackLib_` prefix.
These helpers extend the hack build without replacing the byte-matched base-game implementations.
The skill-expansion combat wrappers are the integration point for hero passives and equipment effects.

## Damage results

`HackLibDamageResult` in `lib/combat_result.h` reports a connected hit, a new kill, and actual HP removed.
`hackLib_damageResult(hpBefore, hpAfter, hit)` excludes misses, overkill, HP increases and already-dead victims from damage credit.
A connected hit that removes zero HP still has `hit == TRUE`.
Capture the result immediately after native hit resolution, before kill effects, bursts or other callbacks can change the victim.
Keep it on the stack so nested attacks cannot overwrite another attack's result.

With skill expansion enabled, abilities can call:

```c
HackLibDamageResult result =
    skillExpansion_applyRangedHitResult(attacker, victim, damage);
```

This entry point applies the same bonuses and callbacks as `skillExpansion_applyPveRangedHit` and returns the captured primary-hit result.
Both actors must remain valid for the duration of the call.
The original boolean entry point remains compatible with the native hook.
Use `result.damageDealt` for drains and damage thresholds, `result.killed` for kill credit, and `result.hit` for effects that trigger on a connection.
Do not dispatch kill effects again after calling this entry point.
The melee wrapper also captures a result before secondary effects.
Damage source context now distinguishes weapon, projectile, ability, summon, periodic and secondary hits.
Critical-hit and element metadata remain separate native concerns.
Stock ghost hits still credit the summoning player, while tracked ghost callbacks identify the ghost as the source origin.

## Combat events

Player outgoing melee and ranged resolution share `HackLibCombatEvent` in `lib/combat_event.h`.
Ability, summon, periodic and secondary damage routed through those wrappers retain their source kind and ability row.
The event snapshots the source and damage result immediately after native resolution.
Actor pointers are valid only during synchronous handling and must not be retained for delayed effects.

The shared dispatcher calls kill handlers on a new kill, then hit handlers on a connection, then the resolved handler on both hits and misses.
Existing hero `onKill` and `onHit` callbacks keep that order.
New kits can use `onCombatEvent(playerIndex, event)` after those callbacks instead of adding another wrapper.
Put each proc in one callback to avoid dispatching it twice.
Callbacks receive the captured source context, and nested events restore their parent's context.
Dispatch nesting and source depth are bounded by `HACKLIB_MAX_DAMAGE_DEPTH`.
Outgoing melee events expose `criticalKnown` and `critical` from the native damage calculation.
Misses, absorbed attacks that skip that calculation, ranged attacks and incoming attacks retain unknown critical metadata.
Element metadata is not inferred from damage amounts or effect artwork.

Enemy attacks on players also use `HackLibCombatEvent` through `onIncomingCombatEvent(playerIndex, event)`.
The event collects HP removed through the existing HP hook after native shields and incoming-damage/HP-change callbacks, before the new notification and Strider's Riposte.
Nested healing is excluded from damage credit, and nested incoming hits keep independent totals.
Connected absorbed hits have `hit == TRUE` with zero HP removed, while misses have `hit == FALSE`.
Player deaths notify the victim's incoming handler and do not invoke the outgoing kill-credit handlers.
Incoming notifications do not apply damage or run reductions a second time.
Healing through `player_addHp` does not create an enemy-attack event.
Retaliation triggered by an incoming handler should use `skillExpansion_dealDamage` with a secondary source.
Native melee ripostes retain the retaliating player's origin, independently of the enemy's incoming source.

### Element metadata

`event->source.elements` is a mask of `HACKLIB_ELEMENT_FIRE` and `HACKLIB_ELEMENT_POISON` when `elementsKnown` is TRUE.
Known `HACKLIB_ELEMENT_NONE` means an explicitly non-elemental attack, while unknown metadata leaves the attack unclassified.
These tags describe attack properties, independently of HP removed and status application.
An enemy fire or poison attack retains its native tags even when it misses or is absorbed.
Outgoing native melee tags are captured only when its damage calculation runs: Fireburst adds fire and Venomous adds poison.
Both bits can be present, and the Venomous damage bonus does not establish that poison status was inflicted.
Ranged and custom damage remain unclassified unless explicitly tagged.

`event->appliedStatusElements` is meaningful only when `appliedStatusElementsKnown` is TRUE.
Enemy-to-player resolutions observe the native poison application step and report poison when that step leaves a positive duration.
Refreshing an existing poison counts even if its duration stays the same.
Gandalf's full shield suppression, a miss, or resistance that reduces duration to zero reports no applied poison.
The native two-HP poison contact cost still occurs when the duration is fully resisted.
Fire is an attack property here, not an inferred burning status.
Other status paths retain unknown application metadata.

Use `skillExpansion_dealElementalDamage(attacker, victim, damage, kind, abilityRow, elements)` for an authored skill's declared elements.
It preserves the normal damage route and source-depth rules, and rejects unsupported mask bits before dealing damage.
Tagging alone does not apply poison, add damage, or apply resistance a second time.
Nested attacks created with the ordinary damage API start with unknown elements instead of inheriting the parent's tags.
Use the elemental API explicitly for an elemental secondary proc or periodic pulse.

`make check-elemental-hook` builds the selected hack and executes the patched four-argument status hook and native status routine under Unicorn.
It checks shield suppression, equal-duration refresh, full poison resistance, fire armor, stack restoration and raw-damage argument boundaries.
The test stubs HP/spirit writes and percentage arithmetic, so it supplements normal gameplay testing.
The selected patch set must include skill expansion, and `uv` supplies the optional Unicorn dependency for this command.

## Stuns

`hackLib_applyStun(actor, frames)` in `lib/status.h` applies a stun or extends it to the longer remaining duration.
It rejects null actors, dead actors, nonpositive durations and actors that cannot be stunned.
Callers still check allegiance, collision eligibility and invulnerability using their targeting rules.
Durations are capped at 65535 frames to match the attached visual's u16 duration.
The return value indicates whether the stun changed.
Repeated shorter or equal stuns do not refresh the visual.
For other timed effects, use the bounded registry in `lib/timed_effect.h`.

## Timed effects

`HackLibEffectDef` specifies a permanent effect ID, pulse interval, stacking rule and apply/pulse/remove callbacks.
`hackLib_applyEffect` records the owner, target, duration, magnitude and originating ability row.
Refresh keeps the longer remaining duration and the existing magnitude.
Stack also increments the stack count up to `maxStacks`.
Replace removes the previous effect before applying the new magnitude and duration.
Different owners have independent stacks.
Pulse callbacks run with periodic damage-source context.
Apply and remove callbacks run once per attachment, not once per stack.
Compute stack-sensitive bonuses from `effect->stacks`, and undo persistent changes in `remove`.

The registry supports twelve simultaneous effects across both players.
It lazily allocates one pool and releases it when empty.
Allocation failure returns NULL before changing the target.
The target's native tick continues through a wrapper, and expiry restores it.
Death, owner loss and loadout replacement remove effects.
Retired targets are discarded without dereferencing them or running removal callbacks on their replacements.
Callbacks may deal damage but must not mutate the effect pool or replace actor tick callbacks directly.
Use `hackLib_replaceActorTick` when changing a callback on an affected actor.
Actor attachments validate list membership and wrapper identity, rather than providing general actor generation IDs.

## Persistent ghost summons

`lib/summon.h` owns the existing stock `GhostState` payload view and ghost creation/iteration declarations.
It supports the stock ghost NPC kind, not arbitrary enemy actor layouts.
Summon with `spawnNpc`, then initialize its damage and install an ability-specific tick callback.
That callback can opt into persistence:

```c
hackLib_tickPersistentGhost(ghost, isValidTarget, findNextEnemy);
```

The owner must remain a valid player avatar.
The target predicate must be side-effect free and accept actors from the supplied iterator.
The helper reacquires the nearest valid enemy when its target becomes invalid and recovers the stock post-attack death state.
Without targets it permits an idle summon to survive until expiry.
It always delegates to `actor_npc_tickPassive`, which owns lifetime decrement, actor removal and the owner's summon-count decrement.
Never decrement the owner's count again in the wrapper.
Do not install this callback on stock summons unless repeated attacks are intended for that ability.

`hackLib_nextOwnedGhost(owner, cursor)` iterates usable ghosts and excludes despawning actors and lifetime <= 1.
Setting a ghost's lifetime to 1 reserves it for consumption or expiry and prevents a second ability use from selecting it again before native cleanup.
This iteration policy is used by Ringwraith thralls and the elite ghost abilities.
The helper does not implement following the player, summon-specific factions, actor generation IDs or independent kill-credit rules.

## Summon definitions

`lib/summon_registry.h` supplies a pool of eight tracked summons across both players.
A `HackLibSummonDef` provides spawn, tick, despawn and target-filter callbacks.
The cap applies per owner and definition.
The spawn callback receives the lifetime and caller-calculated inherited damage.
The tick callback receives the nearest eligible target and the recorded damage.
Set `managedLifetime` when the registry should expire the actor.
Otherwise the adapter owns native expiry and count bookkeeping.
The native tick is retained when no custom tick callback is supplied.
Summon ticks run with summon damage-source context.
The pool allocates on demand and allocation failure returns NULL before spawning.
`hackLib_clearOwnerSummons` handles loadout cleanup without decrementing native counts itself.
The shared ghost factory uses adapters for native and persistent ghosts.

## Hero definitions and combat hooks

`SkillExpansionHeroKit` owns both active pages, both passive pages, icons, required levels and lifecycle callbacks.
Custom heroes register their kit through the hero manifest.
The legacy per-hero tables remain the fallback for stock heroes.
`hacks/content/heroes.json` registers native hero IDs, skill kits, stock fallbacks, names, base stats, palettes, animation banks, menu animations and equipment classes.
`make hack` generates the C registrations and native table extensions from that manifest.
The native hook sites remain owned by the Ringwraith patch.
Keep that patch enabled when using expanded character registration.
IDs must be consecutive and native saves support at most sixteen heroes.
New hero implementations must be compiled by an enabled patch.
Do not renumber existing hero IDs.

The lifecycle callbacks are `reset`, `cleanup` and `tick`.
Cleanup runs against a still-live matching owner before rebuilding loadout tables.
Actor replacement clears transient state without dereferencing a retired owner.
Purchased skill levels and the native equipment/stat rebuild remain separate from transient effects.
Do not implement a live hero swap by changing `Player.heroId` alone.

The common combat wrappers dispatch `meleeDamage`, `rangedDamage`, `onHit`, `onKill` and `incomingDamage` through the definition.
`onHit` receives a captured `HackLibDamageResult` and the melee/ranged distinction.
`onKill` runs once for the primary hit's new kill, before `onHit`.
Ringwraith retains its kill logic inside its `onHit` adapter and leaves `onKill` empty to avoid duplicate effects.
`incomingDamage` can transform a positive damage amount and receives the known source actor, or NULL when unavailable.
`onHpChange` is the full resource interception hook for unusual hero rules such as Ringwraith's drain-only healing.
Ordinary heroes leave it empty and use native HP handling.
Unmodified HP costs can use `skillExpansion_original_player_addHp` deliberately, bypassing mitigation.

## Cast results and resource accounting

A new active definition can provide `resolve` instead of a void callback.
First-page custom actives use the corresponding entry in `kit.resolvers`.
These return `SkillExpansionCastResult`: success, invalid target, blocked landing, summon limit, unavailable or interrupted.
Morgul Blade and Shadowstep demonstrate this path.
Legacy void callbacks remain supported.
The installed table points at the shared dispatcher, while original callbacks are retained in transient runtime state.

`skillExpansion_cast` records the Spirit actually deducted at cast start.
The dispatcher revalidates ability-specific requirements at completion.
A failed result refunds that recorded payment once, capped by the resource maximum.
A changed equipment cost or a no-cost debug cast cannot increase the refund.
A cancelled cast is refunded when the native casting flag and cast states have both ended.
Loadout initialization discards pending receipts rather than refunding a previous character or save.
Hero-specific refund housekeeping uses `refundSpirit`.

An optional `SkillExpansionCastPolicy` separates animation pose, native cooldown bit, native cast requirement and an independent per-row cooldown in frames.
Set a native rule to `SKILL_EXPANSION_RULE_NONE` to omit it.
Independent cooldowns begin on successful completion and reset with transient state.
Failed or interrupted casts do not start them.
Expanded definitions use `policy`, and first-page kit skills use `policies[row]`.
A NULL policy preserves the original borrowed-row behavior.
`baseRow` still selects the row used by a borrowed native callback and its fallback description and level curve.

Validate targets, landing positions and allocation requirements before applying irreversible ability effects.
A failure result refunds Spirit but does not undo damage, spawned actors or other changes already applied by the callback.

## Reusable ability helpers

`skill_expansion_abilities.h` exposes:

- `skillExpansion_findTarget`: nearest eligible enemy in the native radius iterator, with an optional extra condition.
- `skillExpansion_burstDamage`: area damage with an exclusion actor, source metadata and optional stun.
- `skillExpansion_launchProjectile`: stock projectile flight with a custom ability damage callback.
- `skillExpansion_summonGhost`: cap checking, damage clamping, owner-count registration and optional persistence.
- `skillExpansion_consumeGhost`: reserves a usable owned summon for native removal exactly once.

Projectile tracking allows eight custom ability projectiles across both players.
The factory returns NULL when tracking or actor allocation is exhausted.
A projectile removes itself after a connected hit and cannot damage a second target through another collision callback in that frame.
Native flight, wall handling, initial art and lifetime remain stock behavior.
The custom impact callback replaces native weapon impact effects.
Do not assume this helper supplies custom art, homing or arbitrary projectile physics.

`hackLib_refreshTimer` and `hackLib_tickTimer` in `lib/timer.h` support refresh-without-shortening and one-shot expiration.
Timer storage belongs to the hero's transient state and must be reset with it.

## Damage source context

Use `skillExpansion_dealDamage(attacker, victim, damage, kind, abilityRow)` when an ability knows its source.
Callbacks can inspect `hackLib_currentDamageSource()` synchronously.
The origin, kind, ability row and nesting depth survive nested effects through stack-local contexts.
Do not retain the returned context pointer after a callback returns.
Known secondary bursts use an explicit secondary kind and stop at the shared depth limit of eight.
Future proc implementations must use the typed entry point so they participate in that limit.
Unknown native projectiles may have no ability row.

## Target queries

`HackLibTargetQuery` in `lib/targeting.h` supports circular ranges, an exclusion actor, a caller-supplied allegiance/eligibility filter and an optional cone.
`hackLib_nextTarget` iterates matches and `hackLib_nearestTarget` selects by squared distance.
Cone facing components are -1, 0 or 1, and `coneCosine` is scaled by 256.
A zero facing vector disables cone filtering.
Radii above 32767 and invalid cone settings are rejected.
Dead and pending-removal actors are excluded.
The shared skill target helper uses this query while retaining its foe and extra-condition checks.
Landing queries remain in `lib/movement.h`.

## Movement safety

`hackLib_canLandAt` checks map bounds and every 8-pixel collision cell touched by the movement box.
It reserves both horizontal orientations of asymmetric boxes and rejects special terrain conservatively.
`hackLib_findLanding` tries the preferred point and nearby offsets.
Shadowstep uses this search and returns a blocked-landing result when it fails.
The helpers validate terrain at the destination, not the entire path, dynamic actor overlap or ledge traversal.
Charges and knockbacks still need appropriate path checks.

## Build-time content checks

`make hack` runs `hacks/tools/validate_abilities.py` on the linked ELF before writing the ROM.
The validator reads compiler-derived layouts from `SkillExpansionValidationSchema`.
It checks custom hero tables, callbacks or resolvers, icons, required-level curves, base rows and passive IDs.
C assertions guard value-curve size and skill/hero save-field limits.
Zero-initialized value curves can be intentional for callbacks that calculate values in C, so validation cannot infer a missing balance formula.
Validation also rejects duplicate permanent skill IDs and invalid cast policies.

## Save identities

Region 6 stores versioned 144-byte records keyed by permanent skill IDs, including both pages' purchased levels, cycle flags and quick bindings.
Region 5 keeps its original offset and 32-byte stride for older builds.
Both records are bound to the native slot checksum.
Legacy records load through the frozen active and passive identity maps and are upgraded on the next save.
Migration does not write SRAM during loading.
Newer or corrupt tagged records do not fall back to stale expansion purchases.
Deleting a native slot clears its extension records.

Set `SkillExpansionActiveDef.id` for a new active and `kit.activeIds` for a new character's first page.
Existing skills may use the frozen legacy IDs.
When moving a skill, preserve its ID explicitly at the destination.
Passive save identities use their permanent passive IDs, so append new IDs and never renumber old ones.
The legacy passive-slot map must remain unchanged when rearranging trees.
Removed IDs are ignored during loading, so content removals need a deliberate refund or replacement policy.

## Combat Lab and feedback

With debug cheats enabled, Mod Options includes a Combat Lab page.
Choose an encounter, set Spawn encounter to Run, then resume gameplay.
Spawning requires a bonus map and no nearby ordinary foes.
This provides test encounters in cleared existing rooms, rather than adding a new mission map.
The available fixtures are a stationary 30000-HP dummy, three ordinary orc archers and one fire orc archer.
The dummy's native archetype max-HP getter remains unchanged, so use ordinary encounters when testing effects based on maximum-HP ratios.
Clear targets removes only tracked lab fixtures.
Refill HP/Spirit is a one-shot debug action.
Lab settings are transient and co-op lab actions are disabled.

Combat display uses the HUD text band for the last actual damage and source kind, summon count and shortest remaining summon lifetime.
Its Casts, Effects, Pools and Memory views show pending casts, results, cooldown frames, effect duration/stacks, pool occupancy, total free heap, largest free block and peak heap use.
Allocations shows allocation failures, the last failed request size and shared text-buffer reuse.
The memory diagnostics API also exposes the caller address of the last failed allocation and invalid-free counts.
CPU views enable a temporary profiler for player updates, passive skills, hero logic, targeting, effects, summons and FX.
Measurements use elapsed display scanlines, including interrupt time, rather than exact CPU cycles.
Targeting and FX also count work items.
Leaving CPU views releases the profiler's heap storage.
Source numbers are 0 unknown, 1 weapon, 2 projectile, 3 ability, 4 summon, 5 periodic and 6 secondary.
Ordinary ability feedback reports insufficient Spirit, unavailable targets, blocked landings, summon limits and fading Ringwraith buffs.
Unchanged feedback stays visible without requesting native text rasterization every frame.
The HUD requests a fresh draw when the message bytes, displayed buffer or color change, after native text or telemetry takes the band, when the native clear flag is set, and after leaving an overlay.
Repeated identical feedback refreshes its lifetime without repeatedly writing its character tiles.
Host HUD regressions cover changed mutable text, pickup priority, expiry and overlay restoration.
Native pickup and interaction text has priority over these messages.

## Implementation checklist

- Keep purchased skill levels separate from temporary levels and bound all five-entry value curves.
- Revalidate targets when a cast resolves and specify the resource policy for failed completion.
- Validate destinations through verified terrain collision code before adding movement abilities.
- Route damage through the shared combat wrappers and preserve player/summon attribution deliberately.
- Reset per-player temporary state on hero/loadout changes and keep serialized state separate from transient effects.
- Use existing FX helpers for attachment, lifetime and palette ownership.
- Verify both granting and removing equipment modifiers, including native derived stats.
- Check the linker RAM budget before adding mutable state.

The linker permits 2768 bytes of permanent mod BSS and forbids initialized writable data because there is no startup copy routine.
It packs BSS against the top of EWRAM and ends the native heap at the actual linked BSS start.
Unused budget remains available to native allocations.
Effects and summons allocate bounded pools only while needed.
Ringwraith marks allocate on demand and release when the kit resets without another Ringwraith player.
Native OBJ text borrows the persistent font scratch buffer for its standard 4bpp rasterization workspace.
Nested calls and larger 8bpp tile buffers retain ordinary heap allocation.
Actor references carry allocation generations, so reusing an actor slot cannot revive an old effect, summon or projectile attachment.
The generation table uses 1184 heap bytes and retains them for the session.
Use the linked section sizes as the current budget, not a hardcoded estimate.
Co-op sends skill tree actions and each slot's page 2 record over the link (`skill_expansion_coop.h`). Other expanded hero state has no transport yet.

## Verification

Run `make check-abilities` with clang or cc available to build the hack and run the engine regressions.
After an existing build, the individual runners are `python hacks/tools/test_ability_support.py` and `python hacks/tools/test_engine_content.py`.
The host tests compile the actual helper headers against the project's actor declarations and generated build headers.
They cover primary damage credit, stun duration rules, persistent retargeting, summon consumption, landing checks, timers and nested damage sources.
The native lifetime/removal function and FX allocation are stubbed there.
They do not replace an emulator playtest of animations, collision, AI or rendering.
Additional host regressions cover circle/cone targeting, effect stacking and cleanup, summon pool exhaustion, allocation failures, stable save validation and legacy migration through reordered skill trees.
They also cover actor-slot reuse, profiler accounting, heap fragmentation and shared text-buffer ownership.

## Saruman expansion fixture

Saruman is hero 9 in the character registry, with the native Saruman sprite and palette, Gandalf-compatible equipment and Gandalf's campaign route and portrait.
His two skill pages contain ten combat actives, a healing skill and eighteen existing passive skills.
His spells exercise independent cooldown policies, periodic burns, a temporary damage-reduction ward, spectral servants, area damage and damage-based healing.
His actives have explicit permanent save IDs.
Spectral servants use the shared native ghost adapter.
Campaign dialogue and the borrowed portrait remain Gandalf's.

## Shared enemy statuses and recipe skills

[Skill delivery recipes](skill-deliveries.md) configure elemental tags, poison, burning, slows, per-level power and cooldowns.
New active definitions can reference a generated delivery directly through `.delivery`, including timeline release and cast refunds.
The shared status API provides targeted cleansing and uses the generation-safe timed-effect pool.
