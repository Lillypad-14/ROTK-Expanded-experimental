# Hero move checks

Run `make check-hero-moves` after generating the hack headers with `make hack-fx` and building the repository's generated includes.
The checks compile actual kit C code with host stubs for native combat, graphics and input APIs.

Coverage includes:

- Frodo: corruption tier boundaries and the 100-to-60 Claim It transition, restored Spirit costs, death cleanup for shadow tint, held Ring, Phial and temporary strike state, and preservation of living characters' Burden during cleanup.
- Sam: levels 1..5 for planting, stew healing/duration and parry damage/duration; Green Thumb, Master Gardener, Stubborn, Fine Seasoning, Lembas Crumbs, Shire Grit, Taters for All and the emergency heal; failed targets and missed-hit secondary effects.
- Smeagol: levels 1..5 for Play Dead, Catch a Fish capacity, Throttle and Slink duration; stealth cleanup, the follow-up strike, failed targets/landings, grip passives, takeover/free-state rewards and bait visual reuse.
- Aragorn: levels 1..5 for Mark, Flame, Ambush and Return duration; Return level/cost restoration, mark target reuse, King's Presence, Blade Reforged and Hands of the King scaling.
- Saruman: Prism colors, refractions, discounts, spell targeting failures, Guard, healing, curse/binding and Sharkey passives; level 1..5 Flame/Guard/healing and Fires of Industry scaling; curse target reuse.
- All four: positive equipment active-level bonuses on custom moves, with the same capped applied level and one extra level used by the stock curves.

The audit fixes secondary effects applying on Sam's missed rope/potato attacks, stale Mark/bait/curse references after actor reuse, omitted equipment level bonuses, and Sam's rope/potato moves inheriting Knife Toss's weapon requirement.
Shared target queries and affected custom bursts use the gameplay actor list and explicit distance checks rather than depending on native radius-iterator behavior.

In mGBA, the audit verified Sam's purchased Green Thumb levels changing growth from 24 points/tick at rank 1 to 40 at rank 5 while tending, plant cast levels including an equipment bonus, level-5 stew healing, and Smeagol Play Dead durations of 75/135 ticks at levels 1/5 and 90 ticks with the equipment bonus at level 1.
These measurements use debug-controlled test state.

Host checks do not validate every native animation, hit roll, equipment combination, map, co-op interaction or visual lifetime.
The suite does not certify all characters or all stock moves.
Full ROM reconstruction requires the matching `agbcc` compiler described in [setup](setup.md). Its local availability is separate from these host checks.
