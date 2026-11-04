# New attack art for a hero (template, draw, slice, rig)

Not the default. A hero's swings start as its enemy's own attack clip cut into ranges
(animation-studio.md, "The default"). Use this only if the user supplies new drawn art.

How to give a hero (first: the Witch-king) brand-new attack animations, drawn by a person or an AI
image tool. Why: borrowed enemy frames cannot reach the stock heroes' smoothness
(vanilla-swings.md). The drawings are the body only; the rig animates the sword over them every tick.

## 1. The sheet

```
uv run python hacks/tools/anim_studio.py template Witch-king swings \
    --poses ready,windup,raised,strike,impact,follow \
    --guides Attack:0,Attack:2,Attack:4,Attack:5,Attack:6,Attack:7
```

Writes `hacks/content/anims/witchking/sheets/swings_template.png` (+ `swings.json`, the layout):
one column per pose, one row per facing. Each cell is 320x320 (the game canvas at 2x), with a pink
ghost of his current pose and a pink cross at his feet. The background is pure blue (#2020ff).

Rows: 0 up (back view, walking away), 1 up-side, 2 side (facing right), 3 down-side, 4 down
(front view, toward the camera). The game mirrors the side rows for facing left.

## 2. Drawing (what to put in each cell)

- **The body only, no sword.** Arms, hands, cloak, crown. The sword is drawn by the rig.
- **One cyan dot (#00ffff) on the sword hand** (where the hilt sits), a few pixels big. The slicer
  reads it and removes it. Without it the rig needs hand keys typed by hand.
- **Feet on the pink cross**, the same scale as the pink ghost (about 80 px tall at 1x, 160 at 2x).
- Keep the blue background. Pinks can stay: the slicer removes them.
- Any colors (PC). Pixel art at exactly 2x (or 1x, 4x...) slices pixel for pixel; other sizes work
  and come out smoothed.
- Pose meaning for a chop (the default sheet): ready (guard), windup (rearing back, hand rising),
  raised (hand high behind the head, the top of the wind-up), strike (lunging, arm coming down),
  impact (deep forward lean, arm extended low in front), follow (settling, arm low).
  A swing reads best when each pose leans more than the last: rear back, then throw the weight in.
- Witch-king look: tall, black tattered cloak that trails and flares with the swing, spiked iron
  crown over an empty hood, armored gauntlets, menacing and heavy rather than quick.

For an AI image tool: give it the template as the image to draw over, and ask for "dark fantasy
pixel art sprite sheet, the Witch-king of Angmar, black tattered cloak, spiked crown, empty hood,
armored gauntlets, no sword in hand, one pose per cell matching the pink guide, feet on the pink
cross, plain blue background, top-down 3/4 view like a Game Boy Advance action RPG". Check every
row faces the right way (row 0 his back, row 4 his front) before slicing.

## 3. Slice

```
uv run python hacks/tools/anim_studio.py slice Witch-king swings path/to/drawn.png
```

Writes `hacks/content/anims/witchking/swings_<pose>_f<facing>.png` (game canvas, origin at the feet)
and the hand dots into `hands.json`. Prints each cell's pixel count and hand position: a cell with
0 px was not drawn, one without a hand needs a dot.

## 4. Rig it

In `hacks/content/anims/witchking.json`, rig bodies take `"png:swings_<pose>"`:

```json
{"rig": {"ticks": 14, "sword": "Attack:6",
         "body": [[0, "png:swings_ready"], [2, "png:swings_windup"], [4, "png:swings_raised"],
                  [6, "png:swings_strike"], [7, "png:swings_impact"], [10, "png:swings_follow"]],
         "angle": [[0, -87], [4, -130, "out"], [5, -136], [7, 12, "in"], [8, 14], [13, 30, "out"]],
         "lunge": [[0, 0], [5, -3], [7, 6, "in"], [8, 6], [13, 3, "out"]],
         "hit": [7], "smear": {"style": "morgul", "on": [6, 7]},
         "flash": {"style": "morgul-flash", "on": [7]},
         "facings": {"4": {"angle": [...]}, "0": {...}}}}
```

The hand comes from the cyan dots, so only angles (per facing) and timing are typed. Then the usual:
`anim_studio.py show`, `lint`, `verify`, `hero_fast.py directions`, `hero_fast.py push` into the game.

Tested 2026-11-02 with a sheet filled from his old frames at 3x: hands exact, the rig builds in every
facing (the test output was deleted: no real art yet).

## Status (2026-11-02)

- Template ready: `hacks/content/anims/witchking/sheets/swings_template.png`. No drawings yet.
- `rig_thrust`, `rig_rising`, `rig_cleave` in witchking.json hold the full combo as rigs (timing, sword
  angles, lunges, smears, all 5 facings). Over the OLD boss frames the sword floats off his drawn arm,
  so the combo uses the pre-rig swings for now. With drawings: swap each rig's `body` frames for
  `"png:swings_<pose>"`, drop the typed `hand` keys (the cyan dots replace them), assign the rig_ anims.
- Tried and failed with the old frames: cutting his arm+sword out and turning it about the shoulder
  (the torso gets mangled, the piece misses the arm). Needs real art.
