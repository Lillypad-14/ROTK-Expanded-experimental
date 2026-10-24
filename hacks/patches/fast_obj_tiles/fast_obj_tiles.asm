; Replace the ARM search copied by the native startup into g_ObjTileSearchCode.
; r0 = aligned bitmap, r1 = tile count, r2 = first candidate tile.
; Preserve the native byte traversal, including unaligned starting bit positions.
.arm
.org sprite_findFreeObjTiles
.area 0x8c, 0
    stmdb sp!, {r4, lr}
fastObjTiles_restart:
    ; Valid tile indices fit in 16 bits. This also makes -1 return as 0xffff.
    mov r3, r2, lsl 16
    mov r4, r1
fastObjTiles_scan:
    ldrb lr, [r0, r2, lsr 3]
    cmp lr, 255
    bne fastObjTiles_notFull
    ; Inspect a whole word only after finding an occupied byte at its boundary.
    tst r2, 31
    ldreq r12, [r0, r2, lsr 3]
    cmneq r12, 1
    addeq r2, r2, 24
    add r2, r2, 8
    b fastObjTiles_occupied
fastObjTiles_notFull:
    cmp lr, 0
    bne fastObjTiles_mixed
    add r2, r2, 8
    subs r4, r4, 8
    bgt fastObjTiles_scan
    b fastObjTiles_success
fastObjTiles_mixed:
    and r12, r2, 7
    mov lr, lr, lsr r12
fastObjTiles_bit:
    movs lr, lr, lsr 1
    add r2, r2, 1
    bcs fastObjTiles_occupied
    subs r4, r4, 1
    ble fastObjTiles_success
    tst r2, 7
    bne fastObjTiles_bit
    b fastObjTiles_scan
fastObjTiles_occupied:
    add r12, r2, r1
    cmp r12, 1024
    bls fastObjTiles_restart
    mvn r3, 0
fastObjTiles_success:
    mov r0, r3, lsr 16
    ldmia sp!, {r4, lr}
    bx lr
.endarea
.thumb
