; Fixes for bugs in the original game: patches at game addresses.

; Carried-passive uniques, made to work as unique weapons do: dropping until owned, a second copy
; refused, and droppable again once the owned one is gone.
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
