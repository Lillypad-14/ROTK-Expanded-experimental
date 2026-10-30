; Legacy skills (PC edition): Flame of Anor's revive takes over the stock revive's heal.
.if PC_EDITION
; was: mov r0,r10 ; adds r4,#0xd0 ; ... ; bl player_addHp (maxHp / 2)
.if readu16("rotk.gba", 0x08023fda - 0x08000000) != 0x4650 || readu16("rotk.gba", 0x08023fea - 0x08000000) != 0xf002
    .error "revive heal: unexpected native instructions"
.endif
.org 0x08023fda
    nop
    ldr  r3, [pc, #0]
    bx   r3
    .word legacySkills_reviveHook|1
.endif
