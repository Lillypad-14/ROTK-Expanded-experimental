# ROTK Expanded v0.7 (experimental)

Apply `ROTK-Expanded-experimental-v0.7.bps` to your own unmodified USA/Europe base ROM with SHA-1 `ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd`. Use a BPS patcher such as [Rom Patcher JS](https://www.marcrobledo.com/RomPatcher.js/). Do not patch an older Expanded ROM. Back up your save before trying an experimental build.

## New since v0.6

- Eowyn gains the experimental Against the Dark second skill page: six active abilities, nine passive IDs, Odds tiers and the Rohirrim Gallop mechanic.
- Includes HP-cost strikes, a pull/stun, Gallop lunge, defensive ally support, Odds-based healing and I Am No Man!.
- Adds lifecycle, combat, stock-callback and HUD integration for the new kit. Eowyn currently uses stock icons.
- Includes the PC host-capacity compatibility helper and debug stress-spawn support. This release remains a GBA ROM patch, not the PC recompilation.
- Updates the hero guide, current status and generated source inventory. Existing Gauntlet, shop, 94-boon and map-exclusion changes remain included.

## Validation and limits

- `make check-mod` passed: 558 original-game source regions and the reconstructed whole-ROM SHA-1 match the base ROM; 297 core tests passed (three skipped), 96 tool tests passed, and shared runtime/content regressions passed.
- All 70 active definitions passed the linked-content validator.
- Eowyn and Aragorn completed 900-tick-per-row mGBA arena sweeps with no hangs, reported row problems, allocation failures, invalid frees or damaged BSS guards. The fixture included player deaths/reloads and conditional skills that were not castable, so this does not certify every new ability or balance choice.
- BPS application to the clean base was compared byte for byte with the production ROM. Arena automation is absent from the release ROM.

Eowyn's complete manual combat, visuals, balance and co-op testing remain pending. The v0.6 all-hero/all-map audit predates this kit and has not been repeated in full for v0.7. This is an experimental prerelease.

No ROM or BIOS is distributed.
