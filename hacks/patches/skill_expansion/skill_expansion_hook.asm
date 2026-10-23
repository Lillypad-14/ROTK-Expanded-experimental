.thumb
; Cast state pose store. In: r2 = castingActiveSkill, r7 = actor, r10 = playerIndex.
; Continues at the cast state's shared tail, where r0-r3 and lr are dead.
.align 2
skill_expansion_cast_pose_hook:
    mov  r0, r10
    mov  r1, r2
    bl   skillExpansion_getCastPose
    mov  r1, r7
    add  r1, #0x95
    strb r0, [r1]
    hookReturn actor_player_tick + 0x1cb2

; Level-up point grant. In: r1 = new level (0-based), r3 = &skillPoints, r4 = &attributePoints.
; Levels from SKILL_EXPANSION_DOUBLE_POINTS_LEVEL grant a second skill point.
; r0 and r1 are dead at the return, r2-r6 stay live for the level-up loop.
SKILL_EXPANSION_DOUBLE_POINTS_LEVEL equ 20

.align 2
skill_expansion_level_up_points_hook:
    ldrb r0, [r4]
    add  r0, #5
    strb r0, [r4]
    ldrb r0, [r3]
    add  r0, #1
    cmp  r1, #SKILL_EXPANSION_DOUBLE_POINTS_LEVEL
    bcc  @@store
    add  r0, #1
@@store:
    strb r0, [r3]
    ldr  r0, =((player_awardXp + 0x20a)|1)
    bx   r0
    .pool
