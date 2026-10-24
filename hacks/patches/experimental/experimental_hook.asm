.thumb
; Frame pacing: wait for the previous frame's commit before a new frame's sprites are written.
framePacing_commitGfx_hook:
    hookEnter
    bl   framePacing_awaitCommit
    hookLeave
    ; replay sprite_commitGfx's clobbered 8-byte prologue
    push {r4,r5,r6,r7,lr}
    mov  r7,r10
    mov  r6,r9
    mov  r5,r8
    hookReturn sprite_commitGfx + hookTrampolineSize

framePacing_composeOam_hook:
    hookEnter
    bl   framePacing_awaitCommit
    hookLeave
    ; replay sprite_composeOam's clobbered 8-byte prologue
    push {r4,r5,r6,r7,lr}
    mov  r7,r10
    mov  r6,r9
    mov  r5,r8
    hookReturn sprite_composeOam + hookTrampolineSize

framePacing_endFrame_hook:
    hookEnter
    bl   framePacing_awaitCommit
    hookLeave
    ; replay sprite_endFrame's clobbered 8-byte prologue:
    ;   push {r4,r5,lr} ; ldr r0,=g_OamShadow ; ldr r4,[r0] ; movs r0,#128
    push {r4,r5,lr}
    ldr  r0,=g_OamShadow
    ldr  r4,[r0]
    mov  r0,#128
    hookReturn sprite_endFrame + hookTrampolineSize
