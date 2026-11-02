; The super game (PC edition): the main menu gets a seventh row, The Two Towers.
.if PC_EDITION
hookReplace scene_mainMenu_drawEntries, superGame_drawEntries
hookReplace scene_mainMenu_moveCursor, superGame_moveCursor
hookReplace scene_mainMenu_drawEntry, superGame_drawEntry
.endif
