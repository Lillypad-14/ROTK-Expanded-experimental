# Mod RAM notes (2026-10-24)

State: `MODCODE_BSS` starts at 0x0203f560 and the hack build is packed against the top, so it has no spare bytes. A new static
fails the link ("modcode .bss exceeds its RAM budget"). `make test-arena` already fails its link for the same reason.
Measured with the Gauntlet probe (hero Aragorn, all 18 maps): heap Room 816 bytes at every room (peakTop includes map
loads), guard intact, no allocation failures. The heap is the game's, `modRam` hands it everything below the BSS start.

Biggest BSS symbols (bytes): skillExpansion_players 1312, skillExpansion_runtime 216, skillExpansion_heroState 176,
ringwraith states 136, projectiles 96, holds 72, tintedSlots 64, gearStats 64, equippedWords 64, Gauntlet 30.

## Cheap options (no gameplay impact), in order
1. Keep new per-feature state out of BSS:
   - screen state (door offers, pick offers) in `g_SceneCurrent.args` (20 bytes, zeroed per scene);
   - meta and options bytes in the save area (`GlobalModOptionsArea`, Gauntlet meta is read straight from SRAM);
   - per-effect state in the effect actor's `as.combat` fields (Fell Beast).
2. Pack flag bytes into bitfields (the Gauntlet struct has several u8 booleans: mapReady, entering, landFails, purged).
3. Move the budget start down 32 to 64 bytes when a feature needs it. Cost: that many heap bytes, of the 816 measured.
   Keep at least 300 bytes of heap Room. Re-measure with other heroes, since only Aragorn was probed.
4. Trim small pools if never full: projectiles (96), holds (72).

## Not now (gameplay or risk)
- `skillExpansion_players` stores a copy of every hero's 13 `ActiveSkill` rows per player, mostly ROM data. Keeping only the
  changing fields would save a few hundred bytes but touches the whole skill system.
- Shrinking ringwraith `states` or heroState blocks changes hero logic.
