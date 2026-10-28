# ROTK Expanded v0.8 (experimental)

Apply `ROTK-Expanded-experimental-v0.8.bps` to your own unmodified USA/Europe base ROM with SHA-1 `ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd`. Use a BPS patcher such as [Rom Patcher JS](https://www.marcrobledo.com/RomPatcher.js/). Do not patch an older Expanded ROM. Back up your save before trying an experimental build.

## New since v0.7

- Co-op support for the mod: the Skill Expansion second page and mod state work for both players, and mod options that cannot change mid-session are greyed out during co-op.
- The co-op map select gains page 2 (bonus maps) and page 3 (campaign chapters), paged with L/R.
- Each of the eight stock heroes can be created with the Vanilla or the Expanded kit (L on the hero select in New Game, Bonus Maps and co-op). The choice is stored per save slot and carried to the co-op partner.
- Versus, a new mod options page for co-op: Friendly Fire (Grueling/Always/Never) and Duel mode, where a co-op mission becomes a match of knockouts, respawns and scoring (first to 3/5/7/10). START on the co-op map select switches Duel on. Enemies are off by default and damage between players is adjustable.
- Save regions are reserved for heroes imported from The Two Towers. The import, legacy items and the Two Towers skill page are PC-recompilation features: on a GBA or emulator they stay hidden and the game plays as before.

## Validation and limits

- `make check-mod` passed: 558 original-game source regions and the reconstructed whole-ROM SHA-1 match the base ROM; 297 core tests passed (three skipped), 96 tool tests passed, and shared runtime/content regressions (including the co-op page 2 record exchange) passed.
- All 70 active definitions passed the linked-content validator.
- BPS application to the clean base was compared byte for byte with the production ROM (SHA-1 `7beed2ac081878e43b48dad5b23d8b591d4161fb`).
- The v0.7 mGBA arena sweeps were not repeated for this release.

Versus has not had a full two-player playtest, and a damaging Expanded skill landing on the Duel opponent has not been verified in real play. The co-op changes have not had the all-hero/all-map audit v0.6 had. This is an experimental prerelease.

No ROM or BIOS is distributed.
