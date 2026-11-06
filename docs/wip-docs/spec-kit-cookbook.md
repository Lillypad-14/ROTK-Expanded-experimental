# Spec kit cookbook (new hero in few steps)

One page to make a spec hero's skills look right. Field reference: `hacks/tools/hero_kit.py` docstring.
Worked example: `hacks/content/heroes/guardofgondor.json`. Deeper look reference only when a recipe
here does not cover it: `fx-catalog.md`.

## The loop (about 3 tool calls per round)

1. Write a design table first and get the user's OK: one line per skill, "what the text says" ->
   "what the player sees". Do not build before the table is approved.
2. Edit the hero's spec JSON. Kit-only changes need no build: `hero_fast.py play` uses them.
3. C changes (spec_kit.c, fx_kit, any patch): `python tools/lab.py rebuild --target expanded` in
   rotk-recompiled. It runs `make hack-pc` first. Then remake the savestate:
   `hero_fast.py state hacks/content/routes/<hero>_moria.csv <frame> --save <sram>`.
4. Look once: `hero_fast.py skills "<Hero>"` (about 1 minute) writes `build/preview/skills_<Hero>.png`,
   one row per skill, cast at level 5 with full Spirit. `--rows 1,5` redoes some rows only. `--foes 12`
   spawns orc archers around the hero first (skills that need targets show nothing without them).
5. Hand over with `hero_fast.py play` and a short "what you should see" table.

## Uniqueness rule (the user's)

Every hero has a signature mechanic nobody else has, and no skill effect is used by two heroes.
If a design needs an effect another hero already uses, add a new effect to spec_kit.c instead. Check
this table first and add the new hero's row.

| Hero | Signature | Effects it owns |
|---|---|---|
| Witch-king | the Black Captain's dread | reflect, dread, undying, heal |
| Uruk-hai | rebirth from the pits | rebirth, devour, tithe, absorb, lifesteal (whelps still use summon) |
| Guard of Gondor | the city's defence | summon (guards), charge, swoop, ward, empower (beacons) |
| Denethor | fire and despair | despair (HP for spell power), rampart, madness |
| Army of the Dead | terror and the grave | fear, phase, zone, drain, root, stampede |
| Mouth of Sauron | treachery | charm, mark, dismay, dash, bounty, barrage |
| Warg | Frenzy stacks (HUD meter) | leap, weaken, bleed, finisher, frenzy |
| Spider | placed web traps | trap, venom, pull, sleep, backstep |
| Orc Captain | the lash: driving foes | lash, shove, taunt, haste, secondwind, cleave |
| Corsair | plunder: kills drop loot | grapple, flurry, blind, plunder, bomb, broadside |
| Uruk Crossbowman | the marksman holds his ground | pierce, fan, cover, ignite, aim, hail |

Effect reference: fear (foes flee, `speed`), phase (intangible, touch damage), zone (lingering, `trail`
pulse every `trailTicks`), drain (zone on the caster, `percent` heals), root (held in place),
stampede (`count` riders from behind), charm (nearest foe in `range` fights for him), mark (+`percent`
damage taken), dismay (hit + `percent` Spirit per foe), dash (he rushes, `speed`, untouchable), bounty
(kills heal `amount` and give `percent` Spirit), barrage (`count` foes in `range` struck `seconds` later),
lash (a line along the facing, `range` long, `radius` to each side, `amount` + `stun`, `trail` drawn
along it every 16 px), shove (cone ahead within `radius`, kicked at `speed`, down for `stun`), taunt
(foes in `radius` flinch `stun`, `amount` percent less damage taken per foe up to `percent`, for
`seconds`), haste (`amount` percent faster walk, `percent` Spirit a second), secondwind (heal `amount`
plus `amount` per foe in `radius`, up to 5), cleave (for `seconds` each melee hit strikes the other
foes in `radius` for `amount` percent, `trail` on each), reflect (for `seconds` an attacker takes
`amount` percent of its blow back, `trail` on it), dread (every `trailTicks` foes in `radius` cower
`stun`, `trail` on the caster, target layers on each), undying (no blow takes him below 1 HP for
`seconds`, then he heals `amount`), absorb (soak the next `amount` damage within `seconds`), lifesteal
(melee hits heal `amount` percent), rampart (blows from farther than `radius` lose `amount` percent),
madness (melee `amount` percent harder, `percent` more damage taken), grapple (hook the nearest foe in
`range`, rope `trail` along the line, haul to it at `speed`), flurry (`count` cuts of `amount` on the
nearest foe, one every `trailTicks`), blind (foes in `radius` miss `percent` of their blows for
`seconds`), plunder (kills drop loot `amount` percent of the time, game's own loot roll), bomb (a
held `trail` keg blows after `seconds`, target layers at the blast), broadside (`count` blasts
`spread` px apart along the facing, one every `trailTicks`), pierce (a bolt along the facing, `range`
long, foes within `radius` of the line hit, `trail` left every tick of its flight), fan (`count` pierces
spread around the facing), cover (a held `trail` shield ahead, `amount` percent less damage within
`radius` of it), ignite (burning bolt at the nearest foe, `trail` in flight; if it dies within
`seconds` its fire hits foes within `radius`), aim (`amount` percent damage per second stood still, at
most `percent`, `trail` each second), hail (bolts drop at random within `radius`, one every
`trailTicks`, no `trail` field). A flight `trail` must look the same from every direction (a glint, not
a bolt pointing one way). Modcode has no initialized data: a
`static` with a nonzero initializer fails the link ("initialised .data unsupported"). Zones and drains play their
target layers on each foe a pulse hits. bolt, burst and blast are basic deliveries any hero may use;
give them a `travel` and `impact` (skill_fx.json key) or they show the generic orange test art.
A ranged hero's basic attack: keep his own shooting clip as the swing (no hitbox needed) and give the
spec a `"primary": {"frame": n, "damage": [5], "speed", "travel", "impact", ...}` block: a bolt loosed
on frame n of every swing (`tickPrimary`, detected by the player's animationBank being the hero's melee
bank in the hero data region). Example: the Uruk Crossbowman (frame 5, his recoil). Add `"arrow": true` to fire the game's own arrow (Legolas's, 32 directions)
instead of the drawn bolt. Its hit is the stock one, the shooter's full attack (weapon damage, Strength,
accuracy, crits, procs), so Strength/crit passives count; `damage` then only feeds the stand-in bolt. Enemy bodies have no bow-hand point, so tickPrimary starts it 12 px ahead and
18 px up. A foe within ~30 px is hit on the arrow's first frame, so test shots into open floor. Swap any
whoosh in the hero's first `sounds` slot for a shot (64 is Legolas's). Test with
`hero_fast.py check "<Hero>" --face left`. Flight sprites drawn at foot level look low: put the art
high on a tall canvas with the anchor below it (the Crossbowman's bolt: 8x32, anchor [4, 22]).
Moria routes press A after the level starts (dialog skips), which also casts the selected skill: a
savestate can already hold that skill's lingering effects (the Spider's six webs, a ring under the
Uruk-hai). They are real casts, not a bug; read the OBJ palette slot table (0x030053d0 tags, refcounts
after it) from the .gbas with `hero_fast.state_ram` to see which palettes are held. Directional skills: test with
`hero_fast.py skills "<Hero>" --face left` so the hero faces the spawned foes.
Area effects take `radius`, `seconds`, `amount`, `status`. Orc archers keep 80+ px away: reaches under
~72 px miss them.

## Pick colors in one call

`hero_preview.py --enemy GondorSoldier --clip ExtraPose20 --all-palettes` draws one frame in all 90
ObjPalettes ids. Green ids 2..8 are fixed slots: always loaded, never clash. Any other `"OBJ:<id>"`
borrows a shared slot and can scramble in busy fights (the Gondor eagle in palette 22 did); hero_kit.py
warns about it. Multicolor stock shapes ignore the slot hue (fx-catalog.md "Multicolor shapes"):
SHOCKWAVE, SHOCKWAVE_ALT, TENDRIL_STAR, DIAMOND_RING, RISING_STREAKS, RING_SPARKLES, BUBBLE_RINGS.
For white arcs use GHOST, for fire arcs NIGHT. EMBER on those shapes looks brown.

## Recipes by skill kind

| Skill reads as | Spec |
|---|---|
| Allies arrive / march in | summon, `"spread": 64`, target layers SMOKE_SMALL WHITE behind (one with `"delay": 0.3`) |
| A barrier around the hero | ward, `{"effect": "<ring sprite>", "seconds": <ward seconds>}`, burst `"impact"` a white ring effect |
| A shout / horn / war cry | burst, sprite effect over the head + OVAL_RING WHITE behind with `delay` 0.25 and 0.5 (waves) |
| A cavalry or beast charge | charge, enemy clip `anim`, `"trail": {SMOKE_SMALL WHITE behind}`, `"trailTicks": 4` |
| A buff that burns / glows | empower, a sprite effect at the cast + timed SPARKLE_RING in the buff color (`"seconds"`) |
| Something flies in and strikes | swoop, `anim` (flight clip), `strike` (strike clip), `seconds` 0.5..0.8, `impact` effect |
| A projectile | bolt with `travel` and `impact` skill_fx keys |
| A hit at the nearest foe | blast, target layers |

Self layers play on the caster at the cast. Target layers play on each summoned ally, the blast or
devoured foe, or a swoop's strike point when it lands. `"delay"` waits at the spot the layer would
have played.

## Custom sprite effects

1. Copy `hacks/tools/gondor_fx_art.py`: one palette dict (at most 15 colors), one function per frame,
   `sheet()` writes `hacks/content/fx/<hero>_<name>.png`. Run it and look at one preview image.
2. Add the assets and effects to `hacks/content/skill_fx.json`: assets share one new palette `id`
   (taken: 0 shadow pool, 1 Sam, 2 Fell Beast, 3 Gondor), `anchor` is the feet point. An effect's
   layers default to following the actor, `"attachment": "fixed"` leaves them at the spot. Keep the
   file's one-entry-per-line style: never rewrite it with `json.dumps(indent=2)`.
3. Split a ring that surrounds the hero into a back half (`"layer": "behind"`) and a front half.
4. A held effect (`"seconds"`) freezes on its last frame, so end the sheet on the pose to hold.

## Traps

- The hero's cast pose is his clip (`body.cast`). Dark cloaks in enemy art read as a "robe" on casts.
  Check `hero_preview.py "<Hero>"` before calling it a bug.
- Shell heredocs mangle `\n` and quotes in Python edits: use the Edit tool or `apply_edits.py`.
- The savestate is per build: after a full build, remake it before `check` or `skills`.
- Make a new hero's state with `--save build/route_start.sram` (a clean save). A save whose slot
  already holds a hero resumes that hero, and the sheet silently shows the wrong one. A new hero's
  route: copy a neighbour's and add a Right press about 40 frames after the last group switch (R).
