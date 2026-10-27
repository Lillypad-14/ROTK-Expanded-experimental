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

; Co-op message SKILL_EXPANSION_MP_MSG in multiplayer_queueMessage's id switch: args bytes 0..7
; become message bytes 4..11. In: r3 = &args copy, r5 = message, both word aligned.
; Continues at the switch's shared tail, which sends the message from r5.
SKILL_EXPANSION_MP_MSG equ 0x0b
SKILL_EXPANSION_QUEUE_SWITCH_TABLE equ multiplayer_queueMessage + 0x4c
SKILL_EXPANSION_QUEUE_SWITCH_END equ multiplayer_queueMessage + 0x27a

.align 2
skill_expansion_mp_pack_hook:
    ldr  r0, [r3]
    str  r0, [r5, #4]
    ldr  r0, [r3, #4]
    str  r0, [r5, #8]
    ldr  r0, =SKILL_EXPANSION_QUEUE_SWITCH_END
    mov  pc, r0
    .pool

; Co-op message SKILL_EXPANSION_MP_MSG in multiplayer_dispatchMessages' id switch.
; In: r6 = message, r7 = nonzero when the partner sent it, r9 = sender's player index.
; Continues at the switch's shared tail, which keeps using r4-r11.
SKILL_EXPANSION_DISPATCH_SWITCH_TABLE equ multiplayer_dispatchMessages + 0x8c
SKILL_EXPANSION_DISPATCH_SWITCH_END equ multiplayer_dispatchMessages + 0x79c

.align 2
skill_expansion_mp_receive_hook:
    mov  r0, r9
    mov  r1, r6
    mov  r2, r7
    bl   skillExpansion_coopReceive
    ldr  r0, =SKILL_EXPANSION_DISPATCH_SWITCH_END
    mov  pc, r0
    .pool

.if HACK_mod_save
; Save/load menu tick: a nonzero skillExpansion_saveMenuTickHook skips the stock tick.
.align 2
skill_expansion_save_menu_tick_hook:
    hookEnter
    bl   skillExpansion_saveMenuTickHook
    cmp  r0, #0
    bne  @@skip
    hookLeave
    ; push {lr} ; sub sp,#16 ; bl multiplayer_getMessageMask
    push {lr}
    sub  sp, #16
    ldr  r3, =(multiplayer_getMessageMask|1)
    bl   @@callR3
    hookReturn scene_saveLoadMenu_tick + hookTrampolineSize
@@callR3:
    bx   r3
@@skip:
    hookLeave
    bx   lr
.endif
