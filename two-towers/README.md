# The Two Towers (GBA)

Everything this project knows about *The Two Towers* (GBA, USA/Europe, game code `ALPE`) and the
tools for studying it. No Two Towers data is in this repository: every tool reads your own ROM.

## Where everything is

| What | Where |
|---|---|
| Level port research (this page) | `tools/chunk_tags.py`, `compare_functions.py`, `dump_level_row.py`, `find_level_code.py` |
| Function matcher: names Two Towers functions after the ROTK ones they mirror | `tools/ttt_match.py` (about 8 minutes, writes `build/ttt/symbols.json`; needs `make` first for `build/game_symbols.inc`) |
| Disassembler with ROTK names (needs the matcher's output and `llvm-objdump`) | `tools/ttt_dis.py` |
| Save reader: every field of a Two Towers save slot, items named with `--rom` | `tools/ttt_save.py` |
| Offline hero import, Two Towers save to Expanded ROTK save | `tools/ttt_import.py` |
| Findings: player struct, save format, text, heroes, items, skill port status | [`docs/SUPER_GAME.md`](https://github.com/Lillypad-14/rotk-recompiled/blob/main/docs/SUPER_GAME.md) in ROTK Recompiled |
| In-game hero import and Two Towers skill page (PC edition) | [`src/two_towers.cpp`](https://github.com/Lillypad-14/rotk-recompiled/blob/main/src/two_towers.cpp) in ROTK Recompiled |

Run every tool from the repository root, for example `python two-towers/tools/ttt_save.py <TTT .sav> --rom ttt.gba`.

## Level port

Goal: a main-menu option in the PC edition that plays through The Two Towers (GBA) levels with the
Return of the King heroes, skills and items. Scope: port the levels (maps, collision, spawns, scripts)
and the enemies a level needs. The PC build reads the player's own TTT ROM at runtime, so no TTT
data ever goes into a repository.

Status (2026-10-26): research only, nothing ported yet. Next step: step 1 of the plan below.

ROMs used (copy them to the repository root as `rotk.gba` and `ttt.gba`, or set `ROTK_ROM` and
`TTT_ROM`):

- ROTK: `rotk.gba` (SHA-1 `ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd`).
- TTT: `ttt.gba`, *The Lord of the Rings: The Two Towers (USA, Europe) (En,Fr,De,Es,It,Nl)*,
  16 MiB, header title `TWO TOWERS`, game code `ALPE`.

## How ROTK loads a level

All of it happens in `scene_gameplay_enter` (0x08031634).

- `MissionTable` (0x0806C0F0, 12-byte rows): palette cycles, mission block (spawn records, waypoints,
  actor groups, script streams; see `include/mission.h`), song id.
- Level asset table at **0x08069A58**, one **0x4C-byte row per mission id**. Almost every field is a
  pointer to a tagged compressed chunk (`decompress_chunk`, 0x08019D54):

  | Offset | Use |
  |---|---|
  | +0x00, +0x04 | tile graphics chunks, decompressed and uploaded via 0x080324C8 with +0x38 |
  | +0x08 | loader 0x0803EEC0 (collision or terrain, to confirm) |
  | +0x0C, +0x10, +0x14 | BG3 world layer: loader 0x0803EF7C (level header, then two more chunks) |
  | +0x18 | loader 0x0803F048 |
  | +0x1C, +0x20, +0x24 | BG2 world layer: loader 0x0803F0BC |
  | +0x28, +0x2C | 0x08034104 |
  | +0x30, +0x34 | 0x08034148 when +0x34 is set |
  | +0x38 | used with the tile graphics upload |
  | +0x3C | 0x08034104 path (to confirm) |
  | +0x44 | BG1 parallax asset (`gfx_loadBgAsset(1, ...)`), tile count at +0x48 (u16) |
  | +0x4A, +0x4B | mode bytes (+0x4B == 4 is a special case) |

  Unused layers are NULL (mission 0 has no +0x10/+0x14).
- Level header (first chunk of the BG3/BG2 loaders): width and height in 32 px cells at bytes 0 and 2.
  World size in 8 px tiles ends up in `g_WorldWidthTiles` (0x03005F90) and `g_WorldHeightTiles`
  (0x03005F88).
- Map format (from the widescreen work, [ROTK Recompiled `docs/WIDESCREEN.md`](https://github.com/Lillypad-14/rotk-recompiled/blob/main/docs/WIDESCREEN.md)): metatiles. Per layer a
  u16 cell map (one cell per 4x4 tiles) and a block table of 16 tilemap entries per cell value:
  `entry = blocks[cell * 16 + (ty & 3) * 4 + (tx & 3)]`. Runtime pointers: BG3 cell map
  `*(u32 *)0x03005FB4`, blocks `*(u32 *)0x03005FBC`; BG2 cell map `*(u32 *)0x0300586C`, blocks
  `*(u32 *)0x03005FC0`; cells per row `*(u16 *)0x03005878`.
- Layers: BG3 and BG2 are the world, BG1 a repeating parallax pattern, BG0 HUD and text.
- Chunk format: an 8-byte tag, byte 0 is 0x60 or 0xE0, bytes 1-2 a size, then type/parameter bytes
  (for example `e0 f8 05 00 04 01 00 02`). `decompress_chunk` dispatches to ARM decompressors:
  pucrunch (0x080002DC, copied to IWRAM 0x030035E0) and LZ77 (0x0800D1D4). ROTK has about 1,200
  such chunks.

## What TTT shares with ROTK

`tools/compare_functions.py`: ROTK functions whose first 12 instructions (calls and literal loads
masked) also appear in TTT:

| Area | Shared |
|---|---|
| graphics, system, math, RNG, menus, EEPROM | 60-100% |
| font, text | about 50% |
| sprites, actors, scenes | about 10% |
| missions | 3 of 81 |
| combat, skills, items, sound driver | about 0% |

The foundation (rendering, memory, tiles and sprites) is shared. The gameplay layer was rewritten
for ROTK, and TTT has a different sound engine.

Not in TTT (`tools/find_level_code.py`, `tools/chunk_tags.py`):

- ROTK's level loaders (0x0803EF7C, 0x0803F0BC, 0x0803F048, 0x0803EEC0), `decompress_chunk` and both
  ARM decompressors, even with calls and literals masked. The level-entry code around the loader
  calls does not match either.
- ROTK's tagged chunk format: about 9 lookalike headers in TTT, 1,200 in ROTK.

So TTT packs and loads its levels its own way. A broad count of BIOS decompression calls and LZ77
headers was inconclusive (random data looks like both).

## Plan

1. **Find TTT's level loader at runtime.** Boot TTT in mGBA to its first level and trace the code
   that fills the BG map buffers and VRAM, which leads to its decompressor and its level table. Set
   up a Ghidra project for TTT (separate from the ROTK project; do not write TTT names into the ROTK
   config).
2. **Decode one TTT level**: tile graphics, palettes, map layout (probably metatiles, the screen code
   is shared), collision, spawn points.
3. **Convert on the PC host.** At level load, decode the TTT data from the player's ROM and write it
   straight into the buffers ROTK expects (cell maps, block tables, tile VRAM, palettes, collision),
   so ROTK's chunk format never has to be produced. The host RAM (64 MiB) holds the TTT ROM easily.
4. **Hook ROTK's level load** (`scene_gameplay_enter` or the loaders above) for one test mission and
   swap in the TTT map, with ROTK enemies placed by hand.
5. Later: TTT mission scripts (spawns, triggers, objectives, text) through a converter, TTT enemies
   and bosses (import sprites, map behaviours onto ROTK AI), music (ROTK tracks or streamed audio),
   and the main-menu entry.

## Files

- `tools/compare_functions.py`: shared-code statistics between the two ROMs.
- `tools/find_level_code.py`: looks for ROTK's level loaders and decompressors in TTT.
- `tools/chunk_tags.py`: counts ROTK-style tagged chunk headers in both ROMs.
- `tools/dump_level_row.py`: prints one ROTK level-table row and the first bytes of each chunk.

Run them from the repository root with `python two-towers/tools/<script>.py`.
