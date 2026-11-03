; Fixes for bugs in the original game (docs/known-bugs.md): hooks at game addresses.

; Arrow effects of unique bows: the fallback effect-word pick (8 bytes from the arrow index read)
; jumps to bugfixes_arrowFx_hook, which resumes at 0x08026F0C.
hookReplace 0x08026eb8, bugfixes_arrowFx_hook

; combat_getFlatDamageBonus: weapon-family bonuses for unique weapons (known-bugs #2)
hookReplace combat_getFlatDamageBonus, bugfixes_getFlatDamageBonus

; player_updateEquipFlags: the main-hand staff flag for unique staves (known-bugs #3)
hookReplace player_updateEquipFlags, bugfixes_updateEquipFlags

; Dwarven Rage forces a real crit, as Warrior's Pool does (known-bugs #4). Both resolvers test
; combatFlags against PLAYER_COMBAT_FLAG_WARRIORS_POOL (1 << 12, built as #0x80 << 5) before rolling
; a crit; the mask becomes WARRIORS_POOL | DWARVEN_RAGE (0x1200, #0x12 << 8), same registers.
.thumb
.org combat_resolvePveAttack + 0x146
    mov r0, #0x12
    lsl r0, r0, #8
.org combat_resolvePvpAttack + 0x222
    mov r0, #0x12
    lsl r0, r0, #8

; player_addCorruption: every tier up to the meter's, none above (known-bugs #5)
hookReplace player_addCorruption, bugfixes_addCorruption

; combat_fragileOnKill: Fragile in any stat slot, and no break without a fragile hand (known-bugs #7)
hookReplace combat_fragileOnKill, bugfixes_fragileOnKill

; Carried-passive uniques (known-bugs #8, #10), made to work as unique weapons do: dropping until
; owned, a second copy refused, and droppable again once the owned one is gone.
; loot_rollBaseItem kept a passive only when its uniquePassivesCollected bit was set, so
; Galadriel's Phial, the Evenstar and the rest never dropped: the branch to the reroll flips.
.org loot_rollBaseItem + 0x212
    bne loot_rollBaseItem + 0x29e
; actor_lootPile_trigger's passive refusal read uniqueWeaponsCollected (Player +0x188) at the weapon
; loot-type base (0x3d); it reads uniquePassivesCollected (+0x18a) at the passive base (0x47).
.org actor_lootPile_trigger + 0xce
    mov r1, #0xc5
.org actor_lootPile_trigger + 0xd8
    sub r0, #0x47
; item_deleteFromInventory cleared bit (base index - 10) - 0x47, which no passive has; the base
; index - 10 is the passive's bit already.
.org item_deleteFromInventory + 0xa0
    sub r0, #0

; item_equipFromBackpack: a replaced item's stats come off for the slot it was in (known-bugs #9)
hookReplace item_equipFromBackpack, bugfixes_equipFromBackpack
