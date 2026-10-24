.thumb
.align 4
gauntlet_compass_hook:
    hookEnter
    mov r0,r5
    bl gauntlet_compassFrame
    cmp r0,#8
    bhs @@native
    ldr r1,=0x0300406c
    strb r0,[r1]
    hookLeave
    hookReturn 0x08028c6e
@@native:
    hookLeave
    ; Replay the overwritten target lookup and actor move.
    ldr r0,=0x03004074
    ldr r1,[r0]
    cmp r1,#0
    beq @@noTarget
    mov r0,r1
    hookReturn 0x08028c3c
@@noTarget:
    hookReturn 0x08028c44
    .pool
