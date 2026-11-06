# ROTK Expanded v0.10 (experimental)

Apply `ROTK-Expanded-experimental-v0.10.bps` to your own unmodified USA/Europe base ROM with SHA-1 `ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd`. Use a BPS patcher such as [Rom Patcher JS](https://www.marcrobledo.com/RomPatcher.js/). Do not patch an older Expanded ROM. Back up your save before trying an experimental build.

This is a GBA release built on v0.9: bug fixes and Ringwraith buffs, with no new content.

## Fixed since v0.9

Uniques and loot:

- The 14 carried uniques can now drop, including Galadriel's Phial, the Evenstar of Arwen and Boromir's Horn. A bug in the original game stopped them dropping at all.
- Carried uniques now work like unique weapons: a second copy is refused, and one can drop again after you delete your copy.
- Saruman now rolls loot from Gandalf's table. He was reading past the end of the loot table, so his drops came from unrelated data.
- Unique weapons now get weapon-family bonuses (Blademaster, Axemaster, Rangemaster), and unique staves count as staves in the main hand. Uniques are a little stronger as a result.
- Unique bows now show their arrow effects.

Character and item bugs:

- Fragile no longer deletes backpack slot 0 instead of the fragile item. It is now found in any stat slot, including on Obsidian items and the Crystal Crook of Jas Mynn.
- Equipping a two-hander no longer subtracts the wrong item's stats.
- Resetting skills now removes their stat bonuses.
- One-level skills can no longer be raised past their maximum.
- Corruption no longer skips tiers on a big hit, and tier 3 no longer gets stuck after a big drop.

Skills:

- Gimli's Dwarven Rage now deals real critical damage. It used to look and sound like a crit but deal normal damage, and it never crit other players.
- Woodsman (Frodo, Sam, Legolas) now works. It adds damage against spiders, wargs and mumakil, including Shelob. It previously did nothing. Points already spent on it start working without a skill reset.

Stats and skill levels already wrong on an existing character are not repaired by this patch.

## Ringwraith buffs

- All of its heals are 25% larger, including drain healing. The heal figures below already include this.
- No Living Man's kills heal 12.5% of health, up from 8%, before its Undying Malice levels.
- Black Breath's base damage is 4, up from 3, and it heals for 87.5% of the damage it drains, up from 60%.
- The Ring of Khamul heals 5% per kill, up from 3%.

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

- `make check` passed: 558 original-game source regions and the reconstructed whole-ROM SHA-1 match the base ROM, and 297 core tests passed (three skipped). `make check-abilities` passed: 96 tool tests, the shared runtime and content regressions, and native tests of the weapon-family, staff, corruption, Fragile, two-hander and Woodsman fixes.
- The patched ROM's loot code was run in a CPU emulator. Over 300,000 item rolls each for Gandalf, Saruman and Frodo, the rates matched the table above, Saruman's item types matched Gandalf's, and all 51 real uniques dropped for Gandalf. Once-only uniques stopped dropping when marked as owned.
- BPS application to the clean base was compared byte for byte with the production ROM.

None of this has had a manual playthrough. Not tested in game or by a dedicated test: picking up, refusing and deleting carried uniques, unique bow arrow effects, the skill reset and one-level skill fixes, and the Ringwraith's balance with its buffs. Dwarven Rage's real crits have not been balance tested, and Foraging's herb pickups still heal fixed amounts rather than the amounts in its text. This is an experimental prerelease.

No ROM or BIOS is distributed.
