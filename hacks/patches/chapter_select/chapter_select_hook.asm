; Co-op level select tick: chapterSelect_coopLevelSelectTick first (L/R opens the chapter pages),
; then the stock tick unless it returned TRUE.
chapter_select_coop_tick_hook:
    hookEnter
    bl   chapterSelect_coopLevelSelectTick
    cmp  r0, #0
    bne  @@skip
    hookLeave
    ; push {r4,r5,lr} ; ldr r5,=g_SceneCurrent ; ldr r0,[r5,#0x10] ; cmp r0,#3
    push {r4,r5,lr}
    ldr  r5, =g_SceneCurrent
    ldr  r0, [r5, #0x10]
    cmp  r0, #3
    hookReturn scene_multiplayerLevelSelect_tick + hookTrampolineSize
@@skip:
    hookLeave
    bx   lr
