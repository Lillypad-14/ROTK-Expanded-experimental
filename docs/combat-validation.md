# Combat validation

For cold-boot automation without menus or an existing save, use the [direct-boot test arena](test-arena.md).

Build with `make check-abilities`, then test `build/rotk_hack.gba` with a separate save copy.
The automated gate covers host combat dispatch, hero wrappers, FX lifetimes, telemetry and linked content.
Emulator testing checks the compiled ARM code and native game behavior.

## Combat Lab

Enter a single-player bonus map and open Mod Options, Combat Lab.
Clear nearby enemies, choose an encounter and run Spawn encounter.
Dummy is stationary with high HP, Group supplies three enemies, and Fire supplies an attacking fire archer.
Use Ability Bench for temporary skill levels and practice Spirit/cooldown settings.

Choose Combat display, Events for accumulated attempts (`A`), connections (`H`), new kills (`K`) and actual HP removed (`D`).
Reset events clears both players' counters and the last-hit display.
Switch to Combat to inspect the last damage amount and source kind.
Counters record outgoing native hit resolutions, including nested procs.
A multihit attack or an area attack can legitimately create several events.
Compare against the skill's contact and repeat rules rather than assuming one event per button press.

| Case | Check |
| --- | --- |
| Melee hit and miss | Each resolution adds one attempt. A miss adds no connection, damage or kill. |
| Projectile | Contact and repeat limits agree with the delivery definition. |
| Skill damage | Source kind is 3 and the event retains the originating ability row. |
| Summon damage | Source kind is 4 and the player receives the intended credit. |
| Periodic damage | Source kind is 5 and each pulse follows its configured interval. |
| Secondary proc | Source kind is 6 and nested attacks terminate without repeated kill credit. |
| Lethal overkill | Damage credit stops at the target's remaining HP. |
| Already-dead target | Further resolutions add no new kill or damage credit. |
| Multiple targets | Count each eligible contact once, subject to the skill's repeat rules. |
| Incoming defense | The incoming handler observes the final player HP change after reductions. A fully absorbed connection still reports a hit. |
| Player death | Incoming notification reports death without invoking outgoing kill-credit handlers. |
| Retaliation | The returned attack retains the player's origin and restores the incoming source afterward. |
| Elemental attack | Fire/poison tags describe the attack even on a miss or absorbed hit. Untagged custom damage remains unknown. |
| Poison application | Positive native duration reports poison, including an equal-duration refresh. Full shield suppression and zero duration report none. |
| Elemental retaliation | Native melee ripostes do not inherit the enemy's fire tag. Explicit secondary tags are retained separately. |

Kind 1 is weapon damage and kind 2 is a projectile without a more specific source context.
The modern hero `onCombatEvent` callback runs after legacy kill/hit callbacks and also receives misses.
The counters capture native results before those callbacks can generate secondary attacks.
The Events counters remain outgoing-only, so incoming attacks do not inflate damage-dealt or kill-credit totals.
Use a hero kit's `onIncomingCombatEvent` hook or a debugger breakpoint to inspect incoming results.
Check `source.elementsKnown` before interpreting its element mask, and `appliedStatusElementsKnown` before interpreting the applied-status mask.

## Timed visuals

Use Effect Preview to play the Ward recipe directly.
It requests a diamond ring for 90 game ticks, approximately three seconds.
Check that it remains the requested shape, loops, and disappears at expiry.
Timeline-owned effects can end earlier when recovery completes or the cast is cancelled.

Exercise several skills against Group and inspect both front and behind effects while moving.
Check for misplaced anchors, lingering effects, palette changes and missing gameplay indicators.
Use the Memory and Allocations displays to inspect failures after repeated spawning and cleanup.
Cosmetic allocation failure must leave damage, Spirit costs and cooldowns correct.

## Remaining playtesting

Debugger-driven function calls do not verify attack input, projectile travel and collision, camera movement, or a complete encounter.
Test those through normal controls as well.
Combat Lab and Ability Bench requests are disabled in co-op, so multiplayer needs a normal two-player session.
