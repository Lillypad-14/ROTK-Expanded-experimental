# Animation studio (PC edition)

## Primary attack swings: THE method (read this first)

This is how every custom primary attack (the B melee combo: swing 1, swing 2, finisher) is made,
for any hero built from an enemy. The user picked it on 2026-11-02 after trying every alternative
("holy wow thats so much better"). Do not invent a new method; copy the worked example and adjust.

**In one line:** the enemy's own attack frames, played in order, with timing polish (wind-up hold,
impact hold, lunge) and one Morgul-style smear, flash and trail per strike, a new drawing on every
tick (no `"hold"`), checked in all 5 facings.

### The worked example: the Witch-king

`hacks/content/anims/witchking.json`, anims `thrust`, `rising`, `cleave`, assigned in heroes.json
(`"swings": ["@thrust", "@rising", "@cleave"]`). All `"speed": 1` (one step = one 30 Hz tick).

| swing | frames (side) | wind-up | strike / hit | follow-through | effects |
|---|---|---|---|---|---|
| thrust (swing 1, 30 f) | Attack 0-7 | 3 and 4 held 2 | 5 lunge 2, 6 = hit, lunge 4, hold 3 | 7 lunge 4 hold 2, 7 lunge 2 hold 2 | hit: smear `morgul` from Attack:4 to tip, flash, 2 trails; next frame `morgul-fade` 0.7 |
| rising (swing 2, 20 f) | Attack 7-10 | 7 held 2, tween 7 to 8 | 8 = hit, lunge 3, hold 3 | 9 hold 2, 10 hold 2 | hit: smear `morgul` from prev, flash 0.8, trails |
| cleave (finisher, 44 f) | Attack 3-7, 14-16 | 3 hold 3, 4 lunge -2 hold 4 | 5 lunge 3, 6 = hit, lunge 7, hold 4, 6 again hold 2 | 7 lunge 6 hold 3, 7 lunge 4 hold 2, 14-16 settle | hit: smear `morgul` x1.4, flash x1.6, second flash, `morgul-fade` |

The table gives the timing as holds (easier to read). In the file every hold is written out as one
entry per tick, each a slightly different drawing (see step 3, "Smooth"): a held wind-up pulls back
and lifts 1 px a tick, a held hit frame fades its smear (`morgul`, then `morgul-fade`, then
`morgul-faint`) and shrinks the flash, a held follow-through glides its lunge toward the next frame.

Up and down (facings 0 and 4): thrust and cleave use the same frames (his front and back frames
read as the same strike). Rising overrides them, because from the front and back his frame 8 only
raises the sword: 7, 8 (wind-up, no tween), 10 (hit, smear from Attack:8), 11, 12, no trails, the
hit box in front of him.

Styles (file key `"styles"`): `morgul` (a 12 px dark `#1d1428` shadow under a 5 px `#7ee0a4` core
with a `#e8fff0` edge), `morgul-faint`, `morgul-fade`, `morgul-flash` (a `#e0ffe8` spark). Trails
`["#24503a", "#16281f"]`. A new hero copies the styles and changes the colors to fit it.

Sound: heroes.json `"sounds"` first entry `{"mix": [[52, 4, 25], [52, 4, 26]]}` (heavy whooshes,
no voice on every swing).

### Step by step for a new hero

Every command runs from the rotk folder as `uv run python hacks/tools/<tool> ...`.

1. **Study the clip in every facing.** `anim_studio.py clips <Enemy>` (frames, hit frames), then
   `anim_studio.py sheet <Enemy> Attack --hero <Hero>` for the side and `--facing 4` / `--facing 0`
   for down and up. Note per facing which frames wind up, which strike (the blade at full
   extension), which follow through and settle. Most attack clips hold one or two strikes.
2. **Make the three anims** with `anim_studio.py new <Hero> <name> Attack:a-b --speed 1`, or copy
   the Witch-king's three into the hero's file and change the frame numbers.
   - Swing 1: the main strike, wind-up to settle, in order.
   - Swing 2: a second run of frames (a second strike, or the end of the first rising back up).
     It should start near the pose swing 1 ends in: a buffered press cuts straight from one to the
     other.
   - Finisher: the main strike again, heavier: a longer raised wind-up, a bigger lunge, a longer
     hold, the smear and flash scaled up, then the clip's settle frames.
   - Never stitch distant frames or play a clip backwards: the body pops.
3. **Polish each swing** with these knobs (the table above has real values):
   - Wind-up: `"hold": 2` on the last one or two wind-up frames (finisher 3 or 4).
   - Strike: `"lunge"` 2 to 4 over the strike and follow-through (finisher up to 7, a small
     negative one on the wind-up). `lunge` moves along the facing. Never `dx`.
   - Hit frame: `"hit": "auto"` (or `true` when the clip has a hitbox on that frame), `"hold": 3`
     (finisher 4).
   - On the hit frame: `{"smear": {"from": "<last frame showing the blade>", "to": "tip"}, "style":
     "morgul"}`, `{"spark": "tip", "style": "morgul-flash"}`, `"trail": {"steps": 2, "color": [...]}`.
   - The frame after: `{"smear": {"from": "prev", "to": "tip"}, "style": "morgul-fade", "scale": 0.7}`.
   - **Smooth: a new drawing on every tick.** The game runs anims at 30 drawings a second, and the
     stock heroes never repeat one inside a swing. A `"hold"` shows the same drawing for several
     ticks, which reads as a low frame rate (the user: "the framerate is low on it"). So author with
     holds (easy to tune), then run `anim_smooth.py <Hero> <anims...>` (`--dry-run` to preview): it
     writes each held tick as its own entry and changes something on each, as listed here:
     - Wind-up ticks: `"lunge"` 1 px further back each tick plus `"dy": -1` (a lift), to about -3.
       Facing up, the pull-back and the lift cancel out: add `"facings": {"0": {"dy": null}}`, and a
       bigger lunge there if two ticks still match.
     - Hit ticks: the first has the hit, the full smear, the flash and 2 trails. The next ones have
       no `"hit"`, the smear one style fainter and smaller each tick, the flash at 0.6 then gone, one
       trail.
     - Follow-through ticks: the lunge steps toward the next frame's value.
     - `anim_studio.py lint <Hero>` must print no `held ticks before the hit` line in any facing.
   - `anim_studio.py show <Hero> <name>` after each change (side view, about 1 s). Read the
     `check:` lines. Open the PNG only to judge the look.
4. **Up (0) and down (4).** `show <Hero> <name> --facing 4`, then `--facing 0`.
   - Start with no overrides. If the side frame list already reads as a swing there, keep it.
   - Otherwise override that facing entry for entry (the same entry count and hit entry, so every
     facing keeps the same timing), with that facing's own wind-up, strike and follow-through
     frames in order: `"facings": {"4": {"src": "Attack:10", "hit": "auto", "draw": [...]}}`.
   - No tweens there (`"tween": null`): from the front a tween shows a sword floating off the hand.
   - Smear from the last frame that shows the blade to the hit frame's tip.
   - `"trail": null` if the afterimages turn into a dark blob.
   - The hit box goes in front of the hero: below the feet facing down, above facing up, e.g.
     `"hit": [-30, 30, -24, 18]` (down) and `[-30, 30, -72, -30]` (up).
   - The diagonals (1, 3) usually follow the side frames. Step 6 checks them.
5. **Assign and verify.** `anim_studio.py assign <Hero> swings @s1 @s2 @fin`, set the swing sound,
   then `anim_studio.py verify <Hero>` (about a minute). These must PASS: combo timing, in game
   relaxed presses, in game mash presses, colors on screen. Lint lines about blade jumps are the
   source art and are fine where a smear covers the jump.
6. **Check every direction in the real game.** `hero_fast.py directions <Hero>` (about 20 s,
   headless) writes `build/preview/directions_<hero>.png`, one row per facing. In each row the idle
   frames (first and last) and the swing must show the same side of the hero: the back facing up,
   the front facing down. If he turns around to attack, the facings are crossed. First check that
   `hacks/content/enemy_facings.json` does not already list the clip (a wrong listing turned the
   Witch-king around). If it does not, the clip is stored the other way round: list it there and
   run directions again. Before listing, confirm with the clip's own hitboxes: a record whose hit
   box sits above the feet is the up attack (anim_studio.py clips, or the frame's boxes). On
   2026-11-03 the GondorSoldier swing looked turned around in the directions sheet but its hitboxes
   (up record hits above, down record below) proved the stock order right: dark sprites fool the eye.
7. **Show the user.** `hero_fast.py play` opens the game. While it runs, edit and
   `hero_fast.py push` to hot-reload (push while the hero is idle). The user decides.

Targets (`anim_studio.py stock`): swing 1 about 24 to 30 screen frames, swing 2 about 20, the
finisher 36 to 44. Recoveries (the combo window) are generated: nothing to do (see Combo feel).

### Do not

Tried on the Witch-king (2026-11-02). Every one looked worse than the method above:
- 3D or procedural bodies (`puppet3d.py`): "looks horrendous" next to the sprite.
- Rigged swords drawn over the old body frames (`"rig"`): the sword floats off the drawn arm.
- Cutting out and rotating the arm: the torso gets mangled.
- The bare clip with no polish (`"swings": [{"frames": [0, 7]}, ...]`): it reads, but flat.
- Reversed clips (his `ExtraPose03` flourish) as a finisher: a cheer from the front.
- Listing a clip in `enemy_facings.json` without the directions check proving the stock clip turns
  the hero around: the Witch-king's listing made him turn his back to attack.
- Tweens in the up and down facings.
- `"hold"` inside a swing: it looks like a low frame rate. Write the ticks out (step 3, "Smooth").

New drawn art (new-hero-art.md) is only for when the user supplies drawings. The rest of this
guide is reference for the steps above.

## The studio

Make and tune animations for heroes built from enemies (heroes.json `"body"`), with no game build and
no rebuild. Everything is a command that prints text and writes a PNG you can look at.

- Tool: `hacks/tools/anim_studio.py` (run with `uv run python`, from the repository root).
- Format reference: the docstring at the top of `hacks/tools/anim_kit.py` (every frame key).
- Data: `hacks/content/anims/<hero slug>.json` (anims), drawings in `hacks/content/anims/<slug>/`.
- Output: `build/anim/*.png` (review sheets), `build/preview/check_<hero>.png` (in-game frames).

## The loop

1. Look at what exists: `clips WitchKing` (timing, hit frames), `sheet WitchKing Attack --hero Witch-king`
   (numbered frames: the labels are the `Clip:i` a frame takes).
2. Start an anim: `new Witch-king slash Attack:0-7 --speed 1`.
3. Edit the JSON (one frame per line), then `show Witch-king slash`. Read the printed frame table and
   `check:` lines, then the PNG. Repeat. A build takes about a second.
4. Compare: `show Witch-king slash --vs WitchKing/Attack` (any clip or `@anim`), same crop and scale.
5. `lint Witch-king` before using it: errors, slides, clipping, facings that disagree.
6. Use it: `assign Witch-king swings @slash @slash2 @finisher`, then `game Witch-king` (the real PC game,
   headless, about 20 s). `--button A` casts instead. Needs the savestate (`hero_fast.py state`).

7. Before calling it done: `verify Witch-king` (about a minute, text only): lint, combo timing against
   the stock heroes, the real game chaining a relaxed and a mashed rhythm, and the `#rrggbb` colors on
   screen. Look at the in-game sheet it names only when a line fails.

While the game is open (`hero_fast.py play`): edit anims or heroes.json body and sounds, then
`hero_fast.py push` sends the change into the running game (no restart). Poses switch the next time
they start. Skill kits are code: restart for those. Push while the hero is idle.

`stock` prints every stock hero's combo timing (the targets in Combo feel). `vanilla_study.py` measures
the stock swings in depth (docs/wip-docs/vanilla-swings.md).

Cheap habits (tokens): prefer text (`verify`, `combo`, `lint`, `hero_fast.py combo`) over images.
Open a PNG only to judge looks, and then the smallest one (`show` of one anim, not `--all-facings`).
Multi-file edits: `hacks/tools/apply_edits.py edits.json` (exact replacements, all or nothing), not
shell heredocs. PC game rebuild: rotk-recompiled `tools/lab.py rebuild --target expanded`.

`show` checks one facing (default 2, the side view). Pass `--all-facings` before `lint`, since each
facing is a different drawing and stock clips differ per facing (hit frames too).

## Reading `show`

The frame table lists each frame: `#n` (position after ranges expand), `entry` (its index in the
JSON `frames` list, which is what you edit), source, hold, start time `t` in game frames, bounds, and
the hitbox. The title line gives the frame data: total frames and ms, and the hit's startup, active
and recovery in game frames.

In the PNG: red box = hitbox, green = hurtbox, yellow circles = hotspots, the small cross = the
origin (the actor's feet). The last cell is an onion skin of every frame (motion arcs, jitter).
The bar under each row is the timeline: one block per step, red where the hit is active.

`check:` lines are things to look at:
- `slides +3,+0 ... ("dx": -3, "dy": 0 holds it in place)`: the feet moved between frames. Paste
  the suggested dx/dy on that frame if the slide is unwanted (a lunge may want it).
- `touches the canvas edge`: the frame is wider than the 192x192 canvas around the origin.
- `facings differ in timing or hit steps` (lint): one facing hits on a different frame.

## Making attacks read well

The game ticks at 30 Hz. A step lasts `speed` ticks (2 screen frames each), `hold` repeats a step, and
every time in the studio is in screen frames (60 per second). Use `"speed": 1` and `hold` for
one-tick control. A hit lands on the first tick of a frame with a hitbox.

- Anticipation: hold the wind-up 3 to 6 game frames before the strike (`"hold": 2` at speed 1).
- Impact: the hit frame is the one with the blade at full extension. `"hit": "auto"` puts the box
  around what moved since the last frame (the blade, a drawn smear), per facing. Check it in the PNG.
- Hitstop: hold the impact frame 1 or 2 extra steps. A held frame hits on its first step only.
- Smear (easy way): `"draw": [{"smear": {"from": "prev", "to": "tip"}, "width": 7, "color": 5, "edge": 1}]`
  fills the sweep from the last frame's blade tip to this one's, around the hand. `"dir": "cw"/"ccw"`
  forces the way round (default the short way), `edge` is the bright leading edge (0 for none).
  Use a thinner one on the frame after the hit. Needs the blade tracked (see Blade tracking).
- Smear (by hand): `{"arc": [cx, cy, r], "from": a, "to": b, "width": 3, "color": 3, "taper": "in"}`.
  Angles: 0 right, 90 down. Pick light colors from `palette`.
- Impact flash: `{"spark": "tip", "size": 5, "color": 2}` on the hit frame (or `[x, y]`, or `"Clip:i"`).
- Afterimages: `"trail": {"steps": 2, "color": [11, 12]}` on the impact and follow-through frames.
- Follow-through and recovery: keep 4 to 10 game frames after the hit, or the swing feels cut off.
  The combo's recovery poses are each swing's last 2 frames, made automatically and slowed to the
  stock length (see Combo feel: they are the window for chaining the next swing).
- Startup guide (player melee): fast 6 to 10 frames, normal 10 to 16, heavy 18 to 28.
- Kitbash a pose: `"patch": [{"from": "Attack:9", "rect": [x0, y0, x1, y1], "at": [dx, dy]}]` copies
  an arm or weapon from another frame; `"erase"` clears a region first; `"rotate"` with `"pivot"`
  at the shoulder swings an arm.
- Different source enemies: `"src": "Orc/Attack:4"` takes another enemy's frame. Its colors are
  indexes into the hero's palette, so check it and `remap` indexes as needed.
- New art: `export Witch-king Attack:5 --name lunge` writes a 192x192 PNG on the canvas (origin at
  pixel 96, 128). Draw over it, then use `{"png": "lunge", "like": "Attack:5"}`. Colors snap to
  the palette; the build warns how many pixels did not match exactly.

## Swings that feel good: why the method works

Learned on the Witch-king (2026-11-02). The first version looked fine in sheets but felt janky and
weightless in game. What fixed it, in order of how much it mattered:

1. **Play the source's own motion, in order.** Pick a run of consecutive frames (`Attack:0-7`). Do
   not play a clip backwards (his reversed `ExtraPose03` flourish looked like a cheer from the front),
   and do not stitch frames from different parts of a clip. A tween turns only the blade, so the body jumps
   between unrelated poses, and that pop is what reads as jank in motion. Use tweens only between
   neighbouring frames.
2. **Anticipation, then a fast strike.** Hold the wind-up (`"hold": 2` on the last two wind-up frames),
   then move fast: big blade jumps on the strike frames are right, as long as a smear covers them.
3. **Hold the impact.** The hit frame `"hold": 3` (finisher 4), then 2-step follow-through holds.
4. **Lunge.** Push the body forward over the strike: `"lunge"` 2, 4, 4, 2 across the strike and
   follow-through frames (finisher up to 7, a small negative one on the wind-up). Without it the swing
   happens in place. Use `lunge`, not `dx`: `dx` moves right in every facing, so a hero facing down
   or up slides sideways (that was the Witch-king's down-facing bug). `lunge` follows the facing.
5. **One big smear on the impact frame.** If the strike frame hides the blade, draw the smear from the
   last frame that shows it: `{"smear": {"from": "Attack:4", "to": "tip"}}`. Add a flash on the tip
   and 2 afterimage trails. Use a file style (`morgul`) so every swing matches.
6. **A weighty sound with variety.** The swing row gets a mix of whooshes, never a voice on every
   swing: `{"mix": [[52, 4, 25], [52, 4, 26]]}` (Aragorn's two heavier whooshes, without his grunts).
   A single short whoosh on every swing sounds thin.
7. **Hitbox from the source's hit frame.** `"hit": true` copies the clip's own hitbox (right in every
   facing). `"auto"` can come out tiny on frames where little moved: check the red box in `show`.
   Stock heroes have no hitbox in their swing frames because code places their hits, so they are no
   model for this.
8. **Timing near stock** (`stock`): swings 20 to 30 frames, finisher 36 to 44, recoveries generated.
9. **Check the down and up facings** as step 4 of "Primary attack swings" says: that facing's own
   frames, entry for entry, no tweens, the hit box in front, then `hero_fast.py directions`.
10. **A finisher from the hero's real strike, made heavier** (longer raised wind-up, lunge 7, hold 4,
   smear scale 1.4, a second flash frame) beats a different clip whose stance does not match. The
   Witch-king's reversed flourish (`ExtraPose03`) looked like a cheer from the front.

Then `verify <hero>`. It must pass in game at both rhythms. Open the `show` PNG only to judge the
look. A blade hidden in one facing skips that facing's smear with a warning; that is fine.

## Combo feel: input buffering and the recovery window (read before any melee combo)

How the game chains swings (actor_player_tick attack state, 0x08024602, every hero but Legolas):

- A B press **during a swing** is buffered. When the swing ends, the next swing starts at once,
  with no recovery pose.
- A B press **during a recovery** chains the next swing at once (only on recovery frames 0..4).
  When the recovery ends with no press, the hero goes idle. The next press then starts
  **swing 1 again**, so the combo "eats" the input.
- Presses during the finisher are ignored. The next press after it starts swing 1.
- Only new presses count. Holding B does nothing.
- The game ends a pose one tick into its last frame. A hold on a pose's last frame is lost. The
  generated recoveries repeat their last frame once as padding for this.

So the recovery poses are the combo window. A short recovery feels unresponsive even when the swings
look great. That was the Witch-king's "no buffering" bug. His recoveries were shown for 4 frames in
game, against about 14 for the stock heroes, so presses at a normal rhythm restarted swing 1.

Stock targets (side facing, screen frames at 60/s):

| Piece | Aragorn | Gimli | Gandalf |
|---|---|---|---|
| swing 1 | 24 (402 ms) | 24 | 30 |
| recovery 1 | 16 (268 ms) | 16 | 10 |
| swing 2 | 20 (335 ms) | 18 | 18 |
| recovery 2 | 16 (268 ms) | 14 | 12 |
| finisher | 36 (603 ms) | 32 | 44 |

Rules for a combo that feels good:

- A recovery is each swing's last 2 frames plus the padding repeat, each held so the real frames
  last `RECOVERY_TICKS` (8 game ticks = 16 screen frames) in all. heroes.json
  `"body": {"recoveryTicks": n}` overrides it for one hero: about 6 for snappy, 10 for heavy. Under 5
  starts eating presses.
- Keep the recovery at 5 frames or fewer (always true for the generated ones). Frames past index 4
  are outside the chain window.
- Swings much shorter than stock (under about 14 screen frames) shrink the buffer window too. Fast
  swings are fine if the recovery stays near stock length.
- Each swing should end near the pose the next one starts from. A buffered press skips the
  recovery, so swing N's last frame cuts straight to swing N+1's first.

Test it, fastest first:

1. `anim_studio.py combo <hero>` (1 s): every piece's length, the same pieces of a stock hero
   (`--vs Aragorn`, the default), and `check:` lines for recoveries too short to chain or swings too
   short to buffer. `lint <hero>` reports the same checks.
2. `hero_fast.py combo <hero>` (about 8 s, the real game, headless): presses B on a schedule
   (`--presses 10,30,50,70`, frames after the savestate) and prints what the game did with each
   press, from the hero actor's combo bytes:
   ```
   f  34  press B   (during recover 1)
   f  35  swing 2    (recover 1 lasted 12 frames)
   ...
   f  91  swing 1    << combo restarted
   ```
   `<< combo restarted` on a press you expected to chain means the recovery is too short. Try a
   relaxed rhythm (one press every 20 to 25 frames) and a mash (every 6). Both should reach the
   finisher. The first run looks for the actor in host RAM (about 20 s) and caches its address in
   `build/fast/combo_actor.txt`. Delete the file after making a new savestate.
   It needs the runtime's RAM trace over host RAM (rotk-recompiled `patches/ram-trace-host.patch`).

## Blade tracking, tweens and spacing

The studio finds the blade (tip and hilt) in every frame. Smears, sparks and tweens use it.

- `tips Witch-king Attack` draws the tracked blade on each frame of a clip. Check it before smearing.
- Wrong or missing? Fix it once for every anim with the file key `"tips": {"Attack:6": [tipX, tipY]}`
  (or `[tipX, tipY, hiltX, hiltY]`, or `{"2": [...]}` for one facing), or on a single built frame with
  `"tip": [x, y]`.
- `show` prints `blade tip travel per frame` (pixels, `-` where untracked) and draws the tip path on
  the PNG. A `check:` line flags a jump far above the typical step and prints the exact tween to paste.
- Tween: `{"tween": {"from": "Attack:4", "to": "Attack:6", "t": 0.5, "body": "Attack:5"}}` is a new
  in-between frame: the body of `body` (default the nearer frame), with the `from` blade turned about its
  hilt `t` of the way to the `to` blade. `"dir": "cw"/"ccw"` picks the way round. A tween is a frame
  like any other (`hit`, `draw`, `hold` all work), and replaces `src`.
- Good spacing: small moves in the wind-up, one or two big ones into the hit (with a smear), then
  shrinking moves in the follow-through.

## Reviewing the whole combo

- `combo Witch-king` builds the chain as the game plays it (idle, swing 1, recovery, swing 2, recovery,
  finisher), prints each part's frame data and the total, and writes `build/anim/combo_<hero>.png` plus
  a real-speed GIF.
- `game Witch-king --combo --window 0 96` mashes attack in the real game and sheets the whole chain.
  `--window a b` picks which game frames after the press to show (default 0 48).

## Rigged swings (not used: the sword floats off the body, see "Do not")

Stock heroes move their weapon every tick along an even arc while the body leans (vanilla-swings.md).
An enemy body cannot do that by itself, so a `"rig"` frame entry generates the swing one tick at a
time: the chosen body frame with its own sword erased, and the sword (the blade of `"sword"`) drawn
in the hand at an angle eased between keys. Nothing ever stops: the sword and the lunge move every tick.

```json
{"rig": {"ticks": 12, "sword": "Attack:6",
         "body":  [[0, "Attack:0"], [2, "Attack:2"], [4, "Attack:4"], [6, "Attack:6"], [8, "Attack:7"]],
         "hand":  [[0, 8, -28], [2, 4, -39], [4, 10, -40, "out"], [6, 18, -4, "in"], [7, 20, 6], [11, 10, 8, "out"]],
         "angle": [[0, -85], [4, -125, "out"], [6, 0, "in"], [7, 12], [11, 30, "out"]],
         "lunge": [[0, 0], [4, -2], [7, 5], [11, 3]],
         "hit": [6], "smear": {"style": "morgul", "on": [5, 6]},
         "facings": {"4": {"hand": [...], "angle": [...]}}}}
```

- `ticks`: frames generated (each lasts one tick at `"speed": 1`).
- `body`: [tick, frame] switches. Pick frames whose arm matches the sword (raised for a wind-up).
- `angle`: degrees, 0 right, 90 down, -90 up (side view, facing right). Keys `[tick, deg, ease]`:
  `"out"` slows into the key (the top of a wind-up), `"in"` speeds into it (the strike), `"inout"`.
  Values may pass 180 to choose the way round.
- `hand`: `[tick, x, y, ease]` keys for where the hilt sits (origin-relative). Use them: tracking
  guesses the hand wrong on many frames. `anim_studio.py tips <hero> <clip>` prints the tracked
  hilts of frames where the sword shows, a good start.
- `lunge`, `hit` (ticks), `smear` (`style` plus `on` ticks), `flash` (a spark at the tip: `style` or
  `color`/`size`, `on` ticks), `trail` (afterimages: `steps`, `color`, `on` ticks), `length` (sword scale).
- `facings`: the same keys per facing ("4" down/front, "0" up/back, "1"/"3" diagonals). Each facing
  needs its own hand and angle keys: a chop seen from the front sweeps differently.
- Example: `@rigtest` in witchking.json (side view tuned). The sword cannot change the drawn arm, so
  choose body frames carefully. For full freedom the body needs new art without the sword.

Checks for any swing:

- `lint` now flags held ticks before the hit and body jumps over 6 px per tick (stock: none and
  3 to 8 at most).
- `hero_fast.py directions <hero>` (about 20 s): the real game, the hero attacking facing each of the
  5 directions, one row per direction labelled with the game's facing byte
  (`build/preview/directions_<hero>.png`). Every row should be the same swing seen from that side.

## Styles: reusable smears and flashes

The file key `"styles"` names layered looks, so a hero's swings share one and a frame says it in one
item. A style is a list of draw items without a shape. `{"smear": {"from": "prev", "to": "tip"},
"style": "morgul", "scale": 0.8}` becomes one smear per layer, widths (and spark sizes) times scale.
Keys on the item override the layers' (a `"dir"`, a `"color"`). The Witch-king's file has `morgul`
(shadow under a green core with a white edge), `morgul-faint`, `morgul-fade` and `morgul-flash` (a spark).
For a new hero, copy a style and change its colors.

## Colors: any color on PC

Every color key (`draw` colors, smear `edge`, `trail`, `remap` targets) takes a palette index 1..15
or `"#rrggbb"`. PNG drawings keep their exact colors too (no snapping). The previews show them.

How it works: the GBA draws a sprite piece in one 16-color bank, and the hero's bank is full. So the
encoder gives each piece (up to 31 per frame) that uses a new color its own 15 colors, and
hero_data.py tags the frame set (`HPAL` table). The PC host draws those pieces in their colors
(rotk-recompiled `docs/HOST_PALETTES.md`, "Piece palettes"). No game build: `hero_fast.py play`.

- Each piece holds 15 colors. A piece with more keeps its 15 most used and snaps the rest to the
  nearest of them. Gradients over a big area may band: keep them to a few steps.
- Colors are RGB555 in game (each channel in steps of 8).
- The Witch-king's swings are the example: Morgul green edges (`#7ee0a4`, `#e8fff0`), shadow wisps
  under them (`#1d1428`, `#1a2e24`), green afterimages (`"trail": {"color": ["#24503a", "#16281f"]}`).
- Check in game: `hero_fast.py check <hero>` and look at the sheet. No new colors in game but fine
  in `show`: run the host with `ROTK_PC_PIECE_PALETTE_LOG=1` (see HOST_PALETTES.md).

## Slots

`assign` slots: `swings` (the melee combo, a list), `swing.N`, `melee.N` (a full 7-pose melee list),
`cast` (all 7 cast poses), `cast.N` (skill row N+1), `move.<pose>` with pose one of knocked-down,
death, getting-up, hurt, idle, knocked-back, walk. A slot takes `@anim` or a stock clip name.

## Traps

- **Enemies drawn in layers (weapon as its own clip).** Some enemies' body clips hold no weapon:
  the game draws a second clip over them. The Uruk's `Attack`, `Stand`, `Walk` and `Stagger` have
  no blade, and `clips <Enemy>` shows `hits -` on `Attack` while an `ExtraPoseNN` of the same frame
  count and speed carries the hit. Signs: a body clip with no hit and no weapon in `sheet`, and
  ExtraPose clips that show only a weapon (or only effects) on an empty canvas.
  Make each pose an anim that draws the layer over the body, frame for frame:
  `{"src": "Walk:3", "over": ["ExtraPose02:3"]}`, and give the attack the layer clip's own hitbox
  per facing (`"hit"` box plus `"facings"`, read from the layer frame's boxes). Match layers by
  looking, not by frame count alone: several clips share a count and speed, and some are other
  weapons or effects. Check every candidate over the body in the side, front (4) and back (0)
  views, one layer at a time. The Uruk (`hacks/content/anims/urukhai.json`): attack = Attack +
  ExtraPose06, idle = Stand + ExtraPose03, walk and hurt = Walk/Stagger + ExtraPose02. Wrong on
  the first try: adding ExtraPose05 to the walk (a big blade swinging out from the body, it
  floated next to him). ExtraPose07, 09, 10 are pink effects, 00 and 08 gore death bursts, 01 and
  04 other weapons.

- **Facing order: 0 is UP (back view), 4 is DOWN (front view)**, 1 up-side, 2 side, 3 down-side.
  Enemy clips and players share it (enemy facing N goes to hero record N unchanged). Measured in game:
  the player's facing byte actor+0xf8 is up 0, up-right 1, right 2, down-right 3, down 4, and the
  Witch-king's walk plays the right way with records unchanged. On 2026-11-02 two wrong "fixes" cost
  hours: labels flipped from eyeballing dark sprites (the lunge then pushed him backwards), then
  `hero_data.py` reversing the records (his walk then played backwards). Never judge direction from
  sprite pictures: ask the user which way the walk goes, or trace actor+0xf8. `test_anim_kit.py`
  guards FACING_NAMES and FORWARD; `show --facing 4` is what the player sees walking down.
- **If a single clip ever plays up for down** while the walk is right, list it in
  `hacks/content/enemy_facings.json` and `hero_clips.Sources` serves it reversed to every tool. None
  is confirmed: the Witch-king's Attack was listed and taken out again, because what looked reversed
  was the lunge pushing the wrong way (fix motion first, then judge the frames). The game keeps the
  same facing byte while attacking as while walking (measured).
- Every facing is built from the same frame list. A frame that only looks right in one facing
  needs `"facings": {"1": {...}}` overrides for the others.
- Explicit hits: once any frame names `hit`, only frames that name it hit. Without any, frames keep
  the clip's own hitboxes (which can sit on different frames per facing).
- `speed` 0 freezes a pose and blocks attacks.
- Tile data per facing is capped at 64 KB (about 30 drawn frames). Split long anims.
- Hero sounds play per swing (heroes.json `"sounds"`, `sound_preview.py`), not per frame.
