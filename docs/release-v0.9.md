# ROTK Expanded v0.9 (experimental)

Apply `ROTK-Expanded-experimental-v0.9.bps` to your own unmodified USA/Europe base ROM with SHA-1 `ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd`. Use a BPS patcher such as [Rom Patcher JS](https://www.marcrobledo.com/RomPatcher.js/). Do not patch an older Expanded ROM. Back up your save before trying an experimental build.

## New since v0.8

- Eowyn's Against the Dark page has its own icons for all 15 skills, replacing the borrowed stock icons.
- Aragorn gains a sixth page-2 active, Paths of the Dead (Dead build): the Army of the Dead charges ahead, hitting and stunning each foe in its path for 12 +6/level. It spends the Flame bar (+50% from a full bar) and grows 20% per ghost of his, up to three. It replaces the stock Herbal Healing that showed in that slot.
- PC recompilation only (ROTK Recompiled): The Two Towers skill page for every Two Towers hero, bought with skill points by The Two Towers' rules, a Legacy Skills mod option, and HD versions of every custom skill icon. On a GBA or emulator these stay hidden.

## Validation and limits

- `make check-mod` passed: 558 original-game source regions and the reconstructed whole-ROM SHA-1 match the base ROM; 297 core tests passed (three skipped), 96 tool tests passed (including a Paths of the Dead test), and shared runtime/content regressions passed.
- All 71 active definitions passed the linked-content validator.
- Eowyn's and Aragorn's page 2 were checked in game. BPS application to the clean base was compared byte for byte with the production ROM.

Paths of the Dead has not had a manual balance pass. Eowyn's kit remains experimental. This is an experimental prerelease.

No ROM or BIOS is distributed.
