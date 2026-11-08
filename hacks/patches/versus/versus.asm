; Versus: hits between players are marked so the duel can scale their damage.
hookReplace combat_resolvePvpAttack, versus_resolvePvpAttack

; In a duel, stock area skills also find the other player (versus_iterateInRadius).
hookReplace actor_iterateInRadius, versus_iterateInRadius

; In a duel, the stock enemy walk also yields the other player (versus_findNextEnemy).
hookReplace 0x080013a8, versus_findNextEnemy

; The PvP blow's damage call goes through versus_computePvpDamage, by way of a veneer in the
; replaced enemy walk's body.
.org 0x080013a8 + 0x10
.align 2
versus_computePvpDamage_veneer:
    ldr r3, =versus_computePvpDamage|1
    bx r3
    .pool
.if readu32("rotk.gba", 0x0800fb70 - 0x08000000) != 0xf9b4f001
    .error "combat_resolvePvpAttack damage call: unexpected native instruction"
.endif
.org 0x0800fb70
    bl versus_computePvpDamage_veneer
