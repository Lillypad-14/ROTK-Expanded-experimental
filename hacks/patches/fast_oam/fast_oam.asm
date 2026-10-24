; Submit one sprite piece without a stack frame.
.thumb
.org sprite_pushOam
.area 0x50, 0
    ldr r2, =g_SpriteAllocCount
    ldrb r3, [r2]
    cmp r3, 127
    bhi fastOam_full
    add r3, 1
    strb r3, [r2]
    lsl r0, r0, 24
    lsr r0, r0, 21
    ldr r2, =g_OamShadow
    ldr r2, [r2]
    add r2, r0
    ldr r0, [r1]
    ldr r3, [r1, 4]
    str r0, [r2]
    str r3, [r2, 4]
    mov r0, 128
    lsl r0, r0, 3
    add r2, r0
    ; The source may overlap the first page, so reload after writing it.
    ldr r0, [r1]
    ldr r3, [r1, 4]
    str r0, [r2]
    str r3, [r2, 4]
    mov r0, 0
    bx lr
fastOam_full:
    mov r0, 1
    neg r0, r0
    bx lr
    .pool
.endarea
