; The player position getter that gfx_updatePlayerCameraScroll reads, replaced to ease the camera.
hookReplace 0x08014004, cameraEase_getPlayerPosition
; The rotk_main game loop, replaced for frame pacing. It never returns.
hookReplace 0x080315f6, framePacing_mainLoop
