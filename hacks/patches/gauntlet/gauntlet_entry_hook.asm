.thumb
.align 4
gauntlet_playerEntry_hook:
    bl gauntlet_createRoomPlayer
    mov r5,r0
    mov r0,#0
    hookReturn scene_gameplay_enter + 0x4a4
