# Skill delivery recipes

`hacks/content/skill_deliveries.json` defines shared straight projectiles and ground bursts.
`make hack-fx` validates the recipes, generates C tables, and writes `build/skill_fx_preview/deliveries.html` alongside the effect gallery and timing charts.
`make check-fx` runs recipe validation and host tests for the actual delivery runtime.
These recipes are opt-in and do not replace any hero's existing moves.

## Recipe rules

Each recipe has a unique `key`, consecutive `id`, short `name`, and five `damage` values for levels 1 through 5.
Append IDs when adding recipes.
All timing uses 30 game ticks per second.

- `kind` is `straight` or `burst`.
- `speed` is 1..16 pixels per tick for straight projectiles, or zero for stationary bursts.
- `lifetime` is 1..300 ticks. A burst with lifetime 1 resolves once on its first tick.
- `radius` is the collision circle radius, 1..128 pixels, measured from actor origins.
- `splashRadius` adds a contact-centered area to straight projectiles, or zero disables splash.
- `maxTargets` caps distinct actor allocations contacted by a delivery, from 1 through 8. Direct and splash contacts share this budget.
- `pierceTargets` ends a straight projectile after contacting that many distinct direct targets. It must fit `maxTargets`. Misses count toward this limit.
- `maxRepeats` caps attempts against each target allocation, from 1 through 8. Misses also consume attempts.
- `repeatTicks` is the minimum interval between attempts on a target. Stationary bursts pulse at this interval, beginning at tick zero.
- `travelEffect`, `trailEffect`, and `impactEffect` reference keys in `skill_fx.json`, or null disables a visual.
- `trailTicks` sets the interval between trail effects.
- `launchSound` and `impactSound` use `none`, `missile`, `fireburst`, or `stun`, mapped in the generator.
- `stunTicks` is 0..300 ticks, applied only on a successful surviving hit.
- `elements` is null for unknown, `[]` for explicitly non-elemental, or a list containing `fire` and/or `poison`. Tags do not apply a status automatically.
- `status` is `none`, `poison`, `burn`, or `slow`. A successful surviving hit attempts application.
- `statusTicks` is 1..1800 when a status is selected, otherwise zero disables it.
- `statusPower` supplies five level values. Poison and burn require 1..21845 damage per pulse before stacking. Slow ignores power.
- `cooldownTicks` is 0..1800 ticks per player and ability row, starting after a successful launch. Failed launches do not consume it. Cosmetic previews bypass it. Delivery Test damage casts share a separate test cooldown.
- `knockback` is 0..32 pixels of immediate displacement away from the delivery's position. Each pixel requires a safe landing. It is not a native knockback animation.
- `stopOnWalls` stops straight projectiles on blocking or special terrain. Map bounds always stop them.

One broad target query per straight projectile tick supplies candidates, then pixel substeps determine contact order and check terrain.
Candidate and contact lists are bounded to eight actors.
Direct targets share the same repeat ledger as splash targets, preventing a duplicate direct-plus-splash attempt on the same tick.
A reused actor slot has a new allocation generation and counts as a new target.

Damage uses the existing ranged-hit resolver and ability damage attribution, preserving hit/miss rules and passive hooks.
Visual allocation failure cannot cancel damage or consume a hit budget by itself.
Travel layers move with the logical projectile and replay after they expire. Trails and impact effects finish naturally.
Clearing a delivery stops its travel layers.
The effect recipes determine their own sprite orientation; the current runtime does not rotate artwork with facing.

## Gameplay API

```c
#include "patches/skill_expansion/skill_delivery.h"

bool started = skillDelivery_launch(
    playerIndex, &SkillDeliveryDefinitions[SKILL_DELIVERY_TEST_BOLT],
    &actor->coords, actor->facing * 4, level, abilityRow, FALSE);
```

For a new active skill, set `.delivery = &SkillDeliveryDefinitions[SKILL_DELIVERY_TEST_VENOM]` on its `SkillExpansionActiveDef` and leave `.resolve` and `.stockCallback` null.
The shared cast runtime launches it at release using the current facing and effective level, handles Spirit refunds on failure, and exposes its cooldown through the existing skill cooldown state.
Keep the ordinary skill row, icon, progression, permanent save ID, cast policy, and optional timeline in the existing definition.
Recipe cooldown and cast-policy cooldown both apply, so the longer restriction wins.
The linked-content check rejects invalid recipe pointers and conflicting custom resolvers.

Alternatively, call from an ability resolver at release, including a cast timeline's release.
Return `SKILL_CAST_UNAVAILABLE` if launch fails so the cast runtime refunds Spirit.
A successful launch completes the cast; a later miss or wall collision does not refund it.
Definitions are copied at launch. Level, damage, ability row and owner generation are retained independently of later bench/loadout changes.
The final argument enables cosmetic-only delivery: collision, travel, impacts and sounds run, while damage, stun and displacement are skipped.

The runtime supports four active deliveries per player.
Each delivery and its target ledger live on the heap. A small per-player context retains telemetry until cleared.
Failure to allocate or exceeding the active limit returns FALSE before spawning graphics or sound.
Gameplay ticks advance deliveries. Overlays pause them. Owner changes, death, hero/loadout reset, or leaving gameplay cancel them when the owner runtime next runs.
Use `skillDelivery_clear(playerIndex)` for explicit cancellation, and `skillDelivery_getStats` / `skillDelivery_resetStats` for telemetry.

## In-game testing

Enter a single-player bonus map, then open Mod Options, Delivery Test.
Choose a recipe, level, placement and mode. Visual only is the default.
Set Fire on resume to Run and resume gameplay to launch once along the player's facing.
On player starts at the player's current position. In front starts 48 pixels ahead.
Ground bursts stay at the chosen position.
Set Clear to Run to cancel this player's active deliveries and reset their telemetry.
Results displays cumulative contacts (`C`), successful hits (`H`), actual direct damage (`D`), and active deliveries (`A`).
Firing a new test resets the counters. Overlapping active tests then contribute to the same counters.
Damage totals exclude damage caused separately by passive or secondary effects.

Use Combat Lab to spawn its existing Dummy, Group, or Fire fixtures.
Visual-only tests report contacts with zero hits and damage.
Damage tests use real combat and can kill targets or trigger passives.
Delivery tests do not spend Spirit, change skill levels, or equip weapons. They are disabled in co-op and outside bonus maps.
The included bolt, piercing bolt, splash bolt, burst and pulse field are authoring examples using existing shared graphics.

## Shared statuses

`skill_status.h` exposes `skillStatus_apply` and `skillStatus_clear` for enemy targets.
Poison pulses every 30 ticks and stacks to three per caster and target.
Burn pulses every 15 ticks and refreshes one instance per caster and target.
Reapplication extends remaining duration to the longer duration, retains the first application's power and attribution, and preserves the pulse schedule.
Different casters have separate instances.
Slow halves ordinary tick displacement, with the strongest movement modifier winning across effects.
It does not modify stored movement stats or animation speed.
Displacements larger than 16 pixels on either axis bypass slowing to preserve teleports.

These custom statuses share the existing 12-slot timed-effect pool.
Pool exhaustion leaves direct hit damage intact and declines the new status.
Dead targets, retired actors, owner generation changes, and explicit owner-effect clearing remove attachments safely.
`skillStatus_clear(target, kind)` cleanses the selected shared status from every caster without clearing unrelated effects.
Callbacks must follow the timed-effect rule against mutating the pool during a pulse.

Periodic damage uses the existing hit resolver with periodic attribution and explicit fire or poison tags.
It can miss or trigger existing defenses and combat hooks.
Custom status durations do not use the native player poison timer or add native poison resistance rules.
The shared framework currently applies to enemies.
The existing stun helper remains available through `stunTicks`.

Venom bolt, Flame bolt, and Slowing burst are opt-in examples in Delivery Test.
Use damage mode to test status application, then Clear to cancel deliveries.
Clear does not cleanse statuses already applied to enemies.
