hookReplace scene_mainMenu_onConfirm, gauntlet_mainMenuOnConfirm

; Replay the player creation call before native camera initialization.
hookReplace scene_gameplay_enter + 0x49c, gauntlet_playerEntry_hook

; Native compass target selection, before the shared arrow renderer.
hookReplace 0x08028c32, gauntlet_compass_hook
