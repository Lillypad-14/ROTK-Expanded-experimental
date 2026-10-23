# Cast timelines and Ability Bench

Run `make hack-fx` to generate effect data and cast timelines.
Open `build/skill_fx_preview/timelines.html` for timing charts and `index.html` for imported graphics.
`make check-fx` exercises the generators, visual runtime and cast/bench integration.

## Authoring timings

Edit `hacks/content/cast_timelines.json`. Each timeline has a consecutive `id`, unique `key`, short `name`, `releaseTick`, `recoveryTicks`, and ordered `events`.
Timing uses 30 game ticks per second. Tick zero runs when the cast starts.
The release resolves the existing ability once, with its damage attribution, target validation, Spirit refund and cooldown rules.
Recovery blocks another active-skill cast. Native animations, movement and melee retain their existing behavior.

```json
{
  "key": "quick",
  "id": 0,
  "name": "Quick",
  "releaseTick": 6,
  "recoveryTicks": 9,
  "events": [
    {"tick": 0, "effect": "preview_ward", "anchor": "owner"},
    {"tick": 6, "effect": "preview_impact", "anchor": "target"}
  ]
}
```

Events reference keys from `skill_fx.json` and use `owner`, `target`, or `position` anchors.
`target` is the nearest eligible target within 160 pixels at cast start, retained with an allocation generation. Missing targets skip that visual cue.
`position` holds the owner's starting position.
These anchors control visuals. The ability's resolver determines actual damage targets at release.
Each cue replaces the previous cue; use a layered effect recipe for simultaneous graphics.
Timeline-owned visuals stop on cancellation or recovery completion, so allow enough recovery for an impact cue to play.
The total duration is limited to 3600 ticks with at most 16 cues.

Opt in with `.timeline = &HackLibCastTimelines[HACKLIB_TIMELINE_QUICK]` on a `SkillExpansionActiveDef`.
For a hero kit's stock rows, set `.timelines` to an array of `SKILL_EXPANSION_ACTIVE_ROW_COUNT` timeline pointers, leaving unmodified entries NULL.
An active definition's timeline takes precedence over the hero kit's row entry.
NULL preserves native timing. The bench can temporarily override either setting.

Opening an overlay pauses the timeline. Hurt, stun, knockback, knockdown, death, or an owner change cancels it.
Cancellation before release refunds paid Spirit. Successful release begins cooldown, and later interruption does not refund the cast.
Late native animation callbacks cannot release a timeline cast again.
Allocation failure prevents a timed cast before charging Spirit; visual allocation failures remain cosmetic.

## In-game testing

With debug cheats enabled, enter a single-player bonus map and open Mod Options, Ability Bench.

- **Skill** selects Healing or a position on skill-tree page 1 or 2.
- **Test level** temporarily sets the selected skill's applied level, from 1 through 5.
- **Cast timing** chooses Native, Quick (6-tick windup, 9-tick recovery), or Heavy (24-tick windup, 18-tick recovery).
- **Practice** refills Spirit and clears shared expansion cooldowns each gameplay tick.
- **Cast on resume** requests one cast with the selected settings.
- **Cancel cast** interrupts a pending cast and clears its timeline.
- **Reset cooldowns** clears shared expansion cooldowns once.

Resume gameplay to apply requests. A second request while casting reports that the cast is still active.
The bench preserves purchased levels and restores the original applied level after a one-shot test, or after Practice is disabled and the active cast ends.
Changing the selected skill restores its previous level before applying the next one.
Normal equipment, native cooldown flags, summon limits and ability-specific restrictions still apply.
The bench does not equip weapons or change native cooldown flags.
Leaving bonus-map mode or entering co-op disables requests and Practice.

Use Combat Lab on the same map for its existing target fixtures and telemetry.
The [combat validation sequence](combat-validation.md) describes the Events counters, source kinds and expected hit/kill behavior.
Effect Preview remains available for testing a visual recipe without casting a skill.

[Delivery Test](skill-deliveries.md) tests reusable projectile and ground-burst recipes without changing a character moveset.
