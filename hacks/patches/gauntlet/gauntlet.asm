hookReplace scene_mainMenu_onConfirm, gauntlet_mainMenuOnConfirm

; Native compass target selection, before the shared arrow renderer.
hookReplace 0x08028c32, gauntlet_compass_hook
