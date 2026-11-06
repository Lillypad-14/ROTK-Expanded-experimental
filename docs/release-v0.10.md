# ROTK Expanded v0.10 (experimental)

Apply `ROTK-Expanded-experimental-v0.10.bps` to your own unmodified USA/Europe base ROM with SHA-1 `ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd`. Use a BPS patcher such as [Rom Patcher JS](https://www.marcrobledo.com/RomPatcher.js/). Do not patch an older Expanded ROM. Back up your save before trying an experimental build.

This is a GBA bug-fix release built on v0.9. It adds no new content.

## Fixed since v0.9

- The 14 carried uniques can now drop, including Galadriel's Phial, the Evenstar of Arwen and Boromir's Horn. A bug in the original game stopped them dropping at all.
- Carried uniques now work like unique weapons: a second copy is refused, and one can drop again after you delete your copy.
- Saruman now rolls loot from Gandalf's table. He was reading past the end of the loot table, so his drops came from unrelated data.

## Unique drop rates

Orc Heads are not a bug. They are the original game's main fallback when a unique roll fails, so expect far more of them than real uniques.

| Unique-tier roll result (1% of item drops) | Chance |
|---|---|
| Orc Head | 53.84% |
| Gems | 28.19% |
| Orc Drum | 9.28% |
| Any real unique | 8.68% |

That makes a real unique about 1 in 1,152 item drops. Chests cannot drop them, and the Ringwraith finds its own wraith gear instead.

## Validation and limits

- `make check` passed: 558 original-game source regions and the reconstructed whole-ROM SHA-1 match the base ROM, and 297 core tests passed (three skipped). `make check-abilities` passed, including the shared runtime and content regressions.
- The patched ROM's loot code was run in a CPU emulator. Over 800,000 item rolls each for Gandalf, Saruman and Frodo, the rates matched the table above, Saruman's item types matched Gandalf's, and all 51 real uniques dropped for Gandalf. Once-only uniques stopped dropping when marked as owned.
- BPS application to the clean base was compared byte for byte with the production ROM.

The fixes have not had a manual playthrough. In particular, picking up, refusing and deleting carried uniques was not tested in game. This is an experimental prerelease.

No ROM or BIOS is distributed.
