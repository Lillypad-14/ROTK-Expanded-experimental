; Heroes (PC edition): the character stats screen shows a registered hero's own portrait.

; Stats screen portrait palette. was: adds r0,#0xc2 ; movs r1,#0x60 ; movs r2,#0xa0 ; bl gfx_loadBgPaletteColors
; with r0 the hero's stock portrait asset. heroPortrait_statsHook returns to 0x080275d2.
.if readu16("rotk.gba", 0x080275c8 - 0x08000000) != 0x30c2 || readu16("rotk.gba", 0x080275ca - 0x08000000) != 0x2160
    .error "stats screen portrait: unexpected native instructions"
.endif
.org 0x080275c8
    ldr  r1, [pc, #0]
    bx   r1
    .word heroPortrait_statsHook|1

; Mission warps grow to fit low hero bodies (hero_doors.c).
hookReplace actor_missionWarp_tick, heroDoors_missionWarpTick
