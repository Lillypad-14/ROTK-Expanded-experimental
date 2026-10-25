# Bug audit, 2026-10-25

## Changes

- Equipment stat changes preserve one copy of Keen Eye, Fleetfoot, Dwarf-gold, and The One Ring's HP overlay. Exiting a room removes the overlay even when equipment changed after the last effect tick. Full stat rebuilds still reset the overlay baseline.
- A completed cast cannot resolve again through a repeated animation callback. Failed and interrupted casts retain their refund and cleanup behavior.
- Echo Cast distinguishes native Herbal Healing from custom abilities in slot 5, so Ringwraith and Saruman skills in that slot remain eligible.
- Gandalf's Sword of Power and Lightstrike recognize unique swords and staves by their metadata family. Ordinary equipment requirements are preserved. Shield Bash also recognizes the shield family on unique equipment.
- Minas Tirith (Gates) is excluded alongside City Streets because actual screen captures show rainbow-pattern terrain corruption. This avoids the broken rooms without claiming their native loader is repaired.

The available map pool is 28 maps. The 32-entry map table also contains two maps without usable routes and the two excluded Minas Tirith maps.
Door illustrations are shared artwork rather than map previews. The selected door's description names its actual map.
The picker avoids immediately repeating a map and duplicating combat maps in one offer, but has no run-wide history.

## Verification

Host regressions cover incremental equipment changes, immediate room cleanup after a gear change, rebuilt stats, duplicate cast completion, unique equipment family restrictions, and custom slot-5 Echo Cast eligibility.
Native debugger checks also exercise the stat-overlay correction and Sword of Power/Lightstrike with real ROM unique-item metadata.
The base ROM has unique swords and staves but no unique item carrying the shield-family flag; unique Shield Bash compatibility is host-tested against synthetic metadata.

The all-hero ability sweep checks 130 row slots, including empty and conditionally unavailable rows. It covers casting, clearing live effects, resetting the fixture, and recasting.
Eowyn's Rohan Sprint initially outlasted the 360-tick wait; a 900-tick stationary-fixture rerun confirmed that it recasts after its native cooldown expires.
Unavailable rows are not treated as successful ability tests, and player deaths in the attacking group fixture are reported separately.
Shop debugger checks cover buying and selling, failed purchases, full inventory, gem caps, stock persistence, scrolling, repeated tab changes, and preserving the selected door across shop visits.

## Final release checks

The normal release passed 297 core tests (three skipped), 96 authoring-tool tests, all 94 boon regressions, shared ability and memory regressions, native bugfix compilation, and byte-for-byte original-ROM reconstruction.
After the Gates exclusion, all ten heroes completed all 28 available maps with every boon granted: 280 combat-room clears, zero placement failures, zero allocation failures, intact memory guards, and no BIOS or invalid-instruction errors.
The final ROM's test configuration is zeroed. Its bytes were compared with the native debugger test copy after restoring that copy's test configuration.

## Reproduce

```sh
make check-mod
cp build/hack.elf build/rotk_hack_release.elf
uv run python hacks/tools/arena_sweep_all.py --emulator C:/path/to/mGBA.exe --ticks 360 --output build/arena-runs/audit
uv run python hacks/tools/arena_sweep_all.py --no-build --emulator C:/path/to/mGBA.exe --heroes 4 --fixture 0 --ticks 900 --output build/arena-runs/eowyn-cooldown
uv run python hacks/tools/gauntlet_run.py --emulator C:/path/to/mGBA.exe --rom build/rotk_hack.gba --elf build/rotk_hack_release.elf --heroes 0 1 2 3 4 5 6 7 8 9 --rooms 28 --probe --boons --jobs 3 --output build/gauntlet-runs/audit
```

The arena build is separate from a normal release ROM. Use a release ROM and its matching ELF for Gauntlet runs after building an arena.
Map probes deliberately visit every eligible map, so they do not test the distribution of ordinary random choices.
A separate native sample of 200 normal door rolls reached all 29 then-eligible maps before the Gates exclusion; this ruled out a five-map restriction in the picker.
After exclusion, another 200 normal rolls reached all 28 available maps and selected neither excluded Minas Tirith map.

## Limits

Collision and combat-room completion do not certify graphics. All 29 previously eligible maps were captured at a room-clear position, which exposed the Gates corruption despite successful combat probes.
Captures sample particular positions and do not cover every camera position, entrance candidate, effect animation, or water frame.
The automated Gauntlet driver refills HP and clears enemies through the damage path. It checks stability and progression, not combat balance, enemy pursuit, or normal player navigation.
These checks do not replace campaign playthroughs, save compatibility testing with every historical version, or hardware testing.
