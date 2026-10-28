; Legacy items (PC edition): saves keep legacy item handles, memory holds legacy rows.
hookReplace save_serializePlayer, legacyItems_serializePlayer
hookReplace save_deserializePlayer, legacyItems_deserializePlayer
hookReplace scene_saveLoadMenu_drawActionPanel, legacyItems_drawActionPanel
