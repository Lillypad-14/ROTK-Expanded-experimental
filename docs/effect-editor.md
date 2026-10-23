# Effect Studio

Start from the repository root:

```sh
make effect-editor
```

This starts a local server at `http://127.0.0.1:8765` and opens the editor.
Alternatively run `uv run python hacks/tools/effect_editor.py --port 8765` and open the URL yourself.
Use another port if it is already occupied. Ctrl+C stops the server.
No ROM build or GBA compiler is needed.

## Editing

The Sprites tab previews PNG sheets using the importer's converted GBA palette on a 240 by 160 canvas.
Play, pause, step one tick, or scrub the slider. Playback uses 30 game ticks per second.
Sprite playback holds the final frame while scrubbing.
Change frame ticks and anchor coordinates, or click a pixel inside the sprite bounds to make it the anchor.
The cross marks the world position. The rectangle shows the frame bounds.
Zoom uses nearest-neighbor pixel scaling.

Effects edits recipes with up to four layers: sprite or stock shape, color, frame ticks, draw layer and attachment.
Timings edits release, recovery and ordered visual cues. A cue replaces the preceding timeline visual, matching the runtime's single visual handle.
The timing track marks recovery, release, visual cues and the current tick.
Delivery combines a delivery recipe with an optional cast timeline, showing travel, trails and impact cues against one target or a group.
Ground bursts are centered on the first preview target. Straight projectiles travel east from the caster.
Delivery fields include speed, radius, target and repeat budgets, effects, sounds and damage curves.

Duplicate entry appends a draft with a new key and, for recipes, a new consecutive ID.
Undo edit restores the preceding project state, including edits on other tabs.
Entry JSON supports settings outside the forms. Apply JSON validates the entire project before accepting that entry.
Changing an asset or effect key requires updating its references together through an imported project.

## Validation and export

Validate runs the existing sprite, effect, cast-timeline and delivery generators in a temporary directory.
It checks PNG alpha and color limits, frame dimensions, tile budgets, palette identity, references, timing bounds and delivery rules.
Validated sprite metadata refreshes after validation.
Source manifests and PNGs are never overwritten by the server.
Reload source reads the current repository files, including new artwork from another tool or contributor, after confirming that an edited draft can be discarded.

Export bundle downloads a validated ZIP containing:

- `hacks/content/skill_fx.json`, `cast_timelines.json` and `skill_deliveries.json`.
- Referenced PNG sheets and shared palette strips at their relative paths.
- `project.json`, which Import project can reopen.

Drafts remain in the current browser tab. Export before closing it.
The ZIP is a snapshot, so review its changes against the latest source files before applying it, especially while another contributor is editing the same manifests.
Copy only the reviewed changes, then run `make check-fx` and `make hack`.
Opening an exported project requires its referenced PNGs to be present under the repository's `hacks/content` directory.

## Preview limits

Imported sprites use the exact converted frame pixels.
Stock FX are labeled schematic guides, with approximate lifetimes for shapes whose duration is not specified.
Layer animation and ordering can be previewed, but attachment to native actor animations and foreground occlusion need an in-game check.
Travel previews illustrate geometry and contact budgets without terrain, collision boxes, hit rolls, status outcomes, sounds, passive effects or native palette/OAM pressure.
The sound fields edit recipe settings without browser audio playback.
Use Effect Preview, Delivery Test and Ability Bench in mGBA for final validation.

## Tests

Run `make check-effect-editor`.
Python tests verify source isolation, cross-manifest validation and exported bundles.
Node.js tests cover frame playback, timeline replacement and delivery contact budgets.
A browser smoke test runs when Chromium, Chrome or Windows Edge is installed, exercising frame controls, anchors, layers, timing, delivery, validation and rejection of invalid JSON.
