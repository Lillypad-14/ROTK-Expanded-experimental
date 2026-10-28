; Versus hooks.

.thumb

; Friendly Fire, melee. combat_resolvePlayerAttackOnActor + 0xfc is the stock Grueling test
; before combat_resolvePvpAttack (reached when a player's melee hit lands on the other player and
; neither is Smeagol):
;     ldr r0,=g_SaveGlobals ; adds r0,#0x30 ; ldrb r1,[r0] ; movs r0,#0x40   <- clobbered (8 bytes)
;     ands r0,r1 ; cmp r0,#0 ; beq +0x156                                    <- skipped
; +0x10a resolves the hit (r5 = attacker, r6 = victim, both callee-saved), +0x156 skips it.
; lr was saved by the function's prologue and r0-r3 are dead here, so the C call is free.
versus_friendly_fire_hook:
    bl   versus_friendlyFireHits
    cmp  r0,#0
    beq  versus_friendly_fire_skip
    ldr  r3,=combat_resolvePlayerAttackOnActor + 0x10a + 1
    bx   r3
versus_friendly_fire_skip:
    ldr  r3,=combat_resolvePlayerAttackOnActor + 0x156 + 1
    bx   r3
.pool

; Friendly Fire, projectiles and weapon effects. 0x08015424 is the trigger of a hit actor whose
; owner (+0xc) is a player; 0x08015468 is its Grueling test, the same 8 clobbered bytes and the
; same skipped branch. 0x08015478 resolves the hit with r2 = sizeof(Player) and
; r3 = g_PlayerGlobals live, so they are put back after the C call; 0x0801557a skips it.
versus_projectile_friendly_fire_hook:
    bl   versus_friendlyFireHits
    cmp  r0,#0
    beq  versus_projectile_friendly_fire_skip
    mov  r2,#0xd6
    lsl  r2,r2,#1
    ldr  r3,=g_PlayerGlobals
    ldr  r1,=0x08015478 + 1
    bx   r1
versus_projectile_friendly_fire_skip:
    ldr  r3,=0x0801557a + 1
    bx   r3
.pool

; Duel: a fresh mission starts or ends one.
versus_gameplay_enter_hook:
    hookEnter
    bl   versus_gameplayEnter
    hookLeave
    ;   push {r4,r5,r6,r7,lr} ; mov r7,r10 ; mov r6,r9 ; mov r5,r8
    push {r4,r5,r6,r7,lr}
    mov  r7,r10
    mov  r6,r9
    mov  r5,r8
    hookReturn scene_gameplay_enter + hookTrampolineSize
