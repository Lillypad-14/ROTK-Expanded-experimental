# Handoff: hero pipeline (2026-11-02)

PC-only now: the GBA edition is frozen (`make hack` no longer links, ignore it). Build with
`make hack-pc`, then in rotk-recompiled `python tools/lab.py prepare --target expanded --rom
../rotk/build/rotk_pc.gba --elf ../rotk/build/hack_pc.elf` and `python tools/lab.py build --target
expanded --sdl2 ~/rotk-recomp-work/deps/SDL2-2.32.10/x86_64-w64-mingw32`
(this also refreshes the launcher's Expanded patch). If the user's game is open, rename the locked
exes to `.old` first. Reference: howto-heroes-skills.md sections 4 and "Skill specs".

NOTHING IS COMMITTED. Both repos (rotk, rotk-recompiled) hold today's work. Ask before committing.

## Built today (rotk unless noted)
- `hacks/tools/new_hero.py`: enemy -> hero (`--list`, `--enemy`, `--base`, `--group`, `--description`,
  `--portrait` auto, `--remove` last hero). Sets palette and enemy voice (`sounds`). Writes
  `hacks/patches/heroes/<slug>.c` + a `pcOnly` heroes.json entry.
- `hacks/tools/hero_registry.py`: generates every per-hero table from heroes.json (stock skills,
  save IDs, page 2, menu data, sound table `HeroSounds`); validates portraits/descriptions; runs
  `hero_kit.py` for heroes with a spec.
- `hacks/tools/hero_kit.py` + `hacks/patches/heroes/spec_kit.c/.h`: skill kits as data
  (`hacks/content/heroes/<slug>.json`): effects bolt/burst/blast/heal/ward/empower. Page 2 all or
  nothing (5-6 skills + 9 page2Passives). Example: `witchking.json`.
- Hero select screen (Bonus Maps -> empty slot): `hacks/patches/heroes/hero_select.c` (mod scene,
  Gauntlet panel UI, groups on L/R, 6 cards/page, Select = Vanilla/Expanded kit). `hero_grid.c`
  launches it; save menu resumes via `heroSelect_resumeSaveMenu`. Creation checked headless.
- Fixes: hero sound table extended (heroes 8+ read junk: Witch-king audio bug, Ringwraith too);
  skill tree only offers page 2 with content (`pageTwoHasSkills`); stats screen portrait hook
  (`hero_portrait.c`, `heroes.asm` at 0x080275c8, BG3 is 8bpp so 4bpp portrait is expanded).
- `hacks/tools/scan_hero_tables.py`: finds hero-indexed ROM tables; all 13 found are reviewed.
- Also today: Legolas hold-to-charge (`deferBowPress`), Ringwraith heal +25%, TTT pause-menu HUD
  fix (rotk-recompiled `src/rotk_hud.cpp`), launcher patch auto-refresh (`tools/lab.py`), Play.ps1
  stale-patch warning, docs/EXPANDED_EDITION.md "Play does nothing".

## Unverified / next, in order
1. Stats screen portrait (8bpp fix, captured): the Witch-king's helm now draws correctly, but the
   portrait's color-0 background shows as teal with a checker pattern, plus a stray orange line at
   the top. Fix in hero_portrait.c: map pixel index 0 (and the spare column) to a neutral dark color
   instead of bank 6 color 0, e.g. write a chosen gray into palette entry 0x60. Recapture: route
   scratch `r4.csv` (bonus maps -> Witch-king -> Moria -> Start, R, R), frame ~2682.
2. User window test: Witch-king audio (now his own voice), skill tree pages, hero select.
3. Hero audit command (`check_hero.py`): arena sweep every row + frame sheet of hero select, both
   skill pages, stats screen; sound row check.
4. More spec effects (summon, dash, chain, aura, lifesteal, shield), hero-specific passives,
   custom icons from PNG in specs.
5. Lift 16-hero cap (4-bit save field; mod save record has a full byte). Gauntlet picker only 10.
6. Commit (ask the user first).

## Session 2 (2026-11-02, later): fast hero tools
Read howto-heroes-skills.md "Tweak a hero fast" for the tools (hero_check, hero_preview,
sound_preview, hero_fast check/play/cards). Design:
- Hero data region (HERO_DATA 0x09e00000, hacks/tools/hero_data.py): sound rows, copies of
  SfxEntries/SfxVariantSets with mixes appended, per hero fixed body slots (3 banks, menu idle and
  selected, a SpriteAnimation[5] record per pose), then frame sets. Code points only at fixed
  slots, so data tweaks need no rebuild and savestates stay valid (hero_fast.py retargets the
  state's ROM SHA-1 at bytes 8..47 and re-pins the rotk.pc manifest's rom_sha1).
- heroes.json "body" (hero_clips.py): poses as clip names or cuts {clip, frames, hit, speed},
  "swings" for the melee combo. Enemy heroes have no move/melee/cast/menu symbols any more; their
  C file holds stats only.
- Sound row (actor_player_tick): [0] every swing incl. combo follow-ups, [1] hurt, [2] death,
  [3] unread. Mixes: {"mix": [[sfx, count] or [sfx, count, sample], ...]}, new sfx ids 296+.
- Melee combo poses: 0 swing 1, 1 recovery, 2 swing 2, 3 recovery, 4 finisher. The "2 hit" bug
  was the whole attack clip in the recovery poses.
- Fixed this session: Witch-king scream/double swing/walking idle/cast hitboxes; Ringwraith and
  Saruman long voices on every swing and every hit; Eowyn's description was cut off.
- Witch-king hurt/knockdown still use his Stagger clip (= first 8 frames of the ExtraPose03 sword
  flourish), not a flinch: pick a cut with hero_preview.py --enemy WitchKing.
- Skill kit fast path (session 3): HERO_KITS region 0x09d00000 (carved off the modcode end), a
  SkillExpansionKitSlot per hero id (176 bytes, kit inline so the runtime's cached kit pointer stays
  valid). hero_kit.py writes build/generated/hero_spec_kits.c, `hero_kit.py link` compiles it with
  build/modcode_cflags*.txt and links it at the region (hacks/hero_kits.ld, --just-symbols). The
  fast patch of an unchanged spec reproduces build/rotk_pc.gba byte for byte (3.6 s).
  hero_fast.py check --button A casts the skill. Also: routes and the route_start save are local
  (build/route_start.sram is a copy of an earlier scratch save with the 4 test slots).

Headless menu routes: Bonus Maps = 5 Downs from New Game; Start at 450. Keys active low:
A 0x3FE, B 0x3FD, Select 0x3FB, Start 0x3F7, Right 0x3EF, Down 0x37F, R 0x2FF.
