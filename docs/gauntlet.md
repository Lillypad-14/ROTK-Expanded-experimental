# Gauntlet prototype

Open The Gauntlet from the main menu, choose a hero, and begin a run.
The briefing shows the selected map and enemy family before entering.
After rewards and any queued level upgrades, another briefing previews the next room.

## Exploring a room

Each supported map has three separate encounter sectors on connected floor.
The compass points toward the next sector along a route around terrain obstacles.
Approach a clearing to activate its defenders after a short warning.
Activation extends up to 160 pixels from the clearing, including connected approach paths around corners.
This replaces the original 48-pixel trigger reach.
Enemies use fixed positions around that clearing, rather than spawn offsets centered on the player.
The living-enemy cap remains five to eight as the run progresses.

Standing at the entrance does not start the encounters.
Routes guide exploration but do not require touching every waypoint: reaching the sector by another path also works.
Guidance periodically rejoins the nearest visible path and looks ahead, so missing a small waypoint does not keep directing the player backward.
Floor checks keep activation tied to a reachable clearing or its connected approach.
During combat the compass tracks a living defender, keeping the same target until it falls.
While a room settles or rewards open, it holds its bearing instead of spinning.
The first encounter has an arrival pause so the room announcement can be read.

After the final group, the game leaves a short settling period for the last kill and lingering effects.
If another living foe appears during that period, combat resumes.
The room-clear message then appears before healing and the reward screen.

Gauntlet ignores Max Level, extra skill/attribute point, and Max Kills cheats during a run.
Their saved settings remain available in campaign play. Combat cheats retain their selected settings.

## Encounter roles and rewards

Each room shuffles Patrol, Ambush, and Elite Guard across its three sectors, once each.
Patrols have three to five defenders with mixed archetypes where the enemy pool permits.
Ambushes give a longer warning, then release four to twelve defenders from the clearing's fixed positions in quick succession, subject to the living-enemy cap.
Elite Guards have one defender drawn from the strongest available tier, up to one tier above the room's normal limit, with 50% extra HP, plus two to five support enemies.
Sector announcements identify the encounter role.

Reward details show the effect's numbers and current ownership or stat/skill rank before and after choosing it.
Boons are unique (one copy maximum), rather than stacking duplicates.
Long descriptions stay inside the detail panel. Right advances to more text and Left returns to the beginning.

## Map coverage

Sixteen maps currently have connected, collision-checked routes.
Routes favor original enemy and spawner placements where those locations are reachable.
They are stored as ROM coordinates in [gauntlet_routes.c](../hacks/patches/gauntlet/gauntlet_routes.c), with no runtime pathfinding allocation.
Clearance includes the larger 33 by 22 pixel movement footprint used by the custom heroes.
Entrances are aligned to nearby verified floor when the map's native spawn is too close to an edge for that footprint.

Helm's Deep Valley and Morgul Vale remain outside the rotation because their reachable entrance areas did not provide three sufficiently separated sectors with story scripts suppressed.
Their route entries are empty, and both random selection and the map probe skip them.
Story scripts and campaign warps remain suppressed during a run.
This prototype uses existing terrain with its own encounters, rather than restoring each map's campaign objectives.

When adding a route, keep the table in `RoomMaps` order and use world-pixel coordinates.
The first point must lie within 48 pixels of the selected native entrance.
Each sector's path starts at the previous sector's clearing, and its final point is its encounter anchor.
Keep waypoints on connected floor, retain corners, and space straight-path points no more than 32 pixels apart.
Use distinct clearings at least 144 pixels apart and verify them with the larger movement footprint.
An empty first route excludes a map from both normal rotation and the probe.

## Regression runs

```sh
uv run python hacks/tools/gauntlet_run.py --emulator C:/path/to/mGBA.exe \
  --heroes 2 7 9 --rooms 16 --probe --jobs 3 --output build/gauntlet-runs/sector-maps
uv run python hacks/tools/gauntlet_run.py --emulator C:/path/to/mGBA.exe \
  --heroes 0 1 2 3 4 5 6 7 8 9 --rooms 3 --jobs 2 \
  --output build/gauntlet-runs/sector-heroes
```

The automated player waits at each entrance before moving, and reports an unexpected early encounter as a stuck-room failure.
It then follows the route in collision-checked steps, clears groups through the damage path, and exercises rewards, briefings, results and the hub.
This traversal check does not replace manual checks of enemy pursuit, encounter balance or the feel of exploration.

On 2026-10-24, the 16-map probe passed with Aragorn, Smeagol and Saruman, and all ten heroes passed three-room runs.
All 78 room clears reported zero failed landing placements, zero allocation failures and intact memory guards.
A debugger-driven hub check also verified Begin Run, the initial briefing, and entering gameplay with a living hero.
