# Skill effect graphics

Edit `hacks/content/skill_fx.json` and PNG sheets under `hacks/content/fx/`.
Run `make hack-fx` to validate, generate native animation data and build the preview gallery at `build/skill_fx_preview/index.html`.
`make hack` runs the same step automatically.
Run `make check-fx` for importer and effect-runtime regressions without building a ROM.

## Sprite sheets

Each asset declares a unique `key`, a palette `id` from 0 through 31, a `sheet` path relative to the manifest, frame `width` and `height`, and optional `frameTicks` and `anchor`.
Frames run left to right, then top to bottom across an evenly divided sheet.
The anchor is the point placed at the effect's world position, measured from a frame's top-left corner.
Its default is the bottom center.

Export PNG with fully transparent or fully opaque pixels.
The importer converts RGB to the GBA's 5-bit color channels and requires at most 15 opaque colors across the entire animation.
It reserves palette index zero for transparency and preserves opaque black as a separate index.
It rejects partial alpha and excess colors instead of silently reducing the artwork's palette.
Browser previews show the converted colors.

Frame dimensions must be multiples of eight, between 8 and 128 pixels, with at most 2304 decompressed tile bytes per frame.
The importer partitions frames into legal GBA OBJ rectangles.
A sheet supports up to 64 frames and must fit native 16-bit frame tile offsets.
Generated frames have disabled collision boxes.
The gallery includes animated GIFs, frame-strip PNGs and `report.json` with frame counts, tile bytes, compressed asset bytes and palette usage.
GIF timing approximates 30 game ticks per second.

The included shadow-pool sheet is exported from the existing shared shadow-pool effect as an editable example.
Existing hand-authored FX assets remain available.

## Large effects

A frame holds at most 2304 tile bytes, so a larger effect is cut into pieces that play as the layers of one effect.
Give each piece asset its own `anchor`, which may be negative or past the frame (-128..128), so the pieces line up
when they spawn at one position, and share a `palette` strip so they use one OBJ palette slot.

Fell Beast is four 72x64 pieces (a 144x128 canvas) of the stock flying Nazgul, shrunk and darkened.
`hacks/tools/fell_beast_art.py` writes the sheets and `make hack-fx` runs it. They are cut from game art, so they
are generated from your own ROM and ignored by git.

## Effect recipes

An effect has a consecutive `id`, a unique `key`, a short display `name` and one to four simultaneous `layers`.
Append IDs when adding effects.
Each layer chooses either an imported `asset` or a stock `shape` and `color`.
The shape and color names are listed in `hacks/tools/skill_fx.py`.

```json
{
  "key": "example_impact",
  "id": 4,
  "name": "Example impact",
  "layers": [
    {"shape": "spark", "color": "gold", "frameTicks": 2},
    {"asset": "shadow_pool", "attachment": "fixed", "layer": "behind"}
  ]
}
```

`layer` is `front` or `behind`.
`attachment` is `follow` or `fixed`.
`frameTicks` changes playback speed and defaults to the imported asset's speed, or two ticks for a stock shape.
Stock shapes can set a positive `duration` in game ticks to use the native timed effect actor.
Imported sprite effects end with their animation.
Their palettes share native enemy palette slots by reference count and release when the effect actor is freed.
Imported assets may share a palette ID when their complete converted palettes match.
An optional `palette` PNG strip supplies a shared color order for multiple sheets.
Reusing an ID with different converted colors is rejected.

## Gameplay API

```c
#include "lib/skill_fx.h"

hackLib_playEffect(HACKLIB_EFFECT_EXAMPLE_IMPACT, target, NULL, NULL);
```

Pass a world position to detach all layers there, or `NULL` to use each layer's attachment setting.
The return value is the number of successfully spawned layers.
Graphics allocation failures are cosmetic and must not determine damage, cooldown or resource outcomes.
Recipes do not schedule damage or cast callbacks.
Stock-shape layers with a duration loop the same burst animation until that duration expires.
Their shape IDs belong to the burst descriptor table, independently of the native timed status-effect table.
Call them at the appropriate cast, impact, periodic pulse or expiry event in ability code.

For an interruptible visual, retain a zero-initialized `HackLibEffectHandle` in the ability's transient state and pass its address when playing.
Call `hackLib_stopEffect` when that visual should end.
Replaying into a handle stops its previous actors first.
Handles carry actor allocation generations, so cleanup cannot remove an unrelated actor that reused an expired effect's slot.
Recipes and animation data live in ROM.
The playback API has no permanent tracking pool.

FX helpers return NULL for invalid shapes, layers, colors, animation sizes or timing before reserving resources.
Failed actor allocations return their palette reference, and exhausted shared palette references reject additional effects.
Use `make check-mod` to build and validate content and run the FX resource and authoring regressions.

## In-game preview

With debug cheats enabled, open Mod Options and switch to the Effect Preview page.
Choose an effect, choose On player or In front, set Preview on resume to Play, then resume gameplay.
The request runs once and does not spend Spirit, deal damage or require a purchased skill.
Preview requests are disabled in co-op.
Stock shapes are inspected here because the browser gallery only renders imported sheets.

This pipeline does not automatically replace existing ability visuals.

For scheduled windup, release and recovery cues, see [Cast timelines and Ability Bench](ability-bench.md).

For projectiles, splash, ground bursts and hit rules, see [Skill delivery recipes](skill-deliveries.md).

For interactive frame, anchor, layer and timing edits, use [Effect Studio](effect-editor.md).

## Stock ground shockwaves

The shared FX helpers combine stock rows 20 and 21, the lower and upper arcs, into one complete shockwave animation.
Both `FX_SHAPE_SHOCKWAVE` and `FX_SHAPE_SHOCKWAVE_ALT` use it, including fixed-position and timed effects.
The animation keeps the stock 17 frames and palette indices and uses one actor with at most 896 tile bytes per frame.
Regenerate `hacks/lib/fx_shockwave.c` with `uv run python hacks/tools/ground_shockwave.py` using the base ROM.
`hacks/tools/test_fx_resources.py` checks both halves, tile offsets, generated data and shared-helper selection.
