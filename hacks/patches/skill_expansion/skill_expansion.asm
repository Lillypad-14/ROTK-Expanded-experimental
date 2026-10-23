.thumb

hookReplace player_initHeroLoadout, skillExpansion_initHeroLoadout
hookReplace player_resetActiveSkills, skillExpansion_resetActiveSkills
hookReplace player_resetActiveSkillControls, skillExpansion_resetActiveSkillControls

hookReplace skill_passive_addLevels, skillExpansion_passiveAddLevels
hookReplace skill_passive_applyLevels, skillExpansion_passiveApplyLevels
hookReplace skill_passive_removeLevels, skillExpansion_passiveRemoveLevels
hookReplace skill_passive_removeAppliedLevels, skillExpansion_passiveRemoveAppliedLevels
hookReplace skill_passive_removeTemporaryLevels, skillExpansion_passiveRemoveTemporaryLevels
hookReplace skill_passive_addTemporaryLevels, skillExpansion_passiveAddTemporaryLevels
hookReplace skill_passive_applyAllLevels, skillExpansion_passiveApplyAllLevels
hookReplace skill_passive_applyStatsUpToLevel, skillExpansion_applyStatsUpToLevel
hookReplace skill_passive_applyLevelStats, skillExpansion_applyLevelStats
hookReplace skill_addTemporaryLevels, skillExpansion_addTemporaryLevels
hookReplace skill_removeTemporaryLevels, skillExpansion_removeTemporaryLevels

hookReplace skill_active_addLevels, skillExpansion_activeAddLevels
hookReplace skill_active_applyLevels, skillExpansion_activeApplyLevels
hookReplace skill_active_addPurchasedLevels, skillExpansion_activeAddPurchasedLevels
hookReplace skill_active_addTemporaryLevels, skillExpansion_activeAddTemporaryLevels
hookReplace skill_active_addTemporaryLevelsToAll, skillExpansion_activeAddTemporaryLevelsToAll
hookReplace skill_active_removeTemporaryLevelsFromAll, skillExpansion_activeRemoveTemporaryLevelsFromAll
hookReplace skill_active_hasLevels, skillExpansion_activeHasLevels
hookReplaceR3 skill_active_getLeveledValue, skillExpansion_activeGetLeveledValue
hookReplace skill_active_canCast, skillExpansion_activeCanCast
hookReplace skill_active_cast, skillExpansion_cast
hookReplace skill_active_handleCycleCastModeInput, skillExpansion_handleCycleCastModeInput
hookReplace skill_active_handleQuickCastModeInput, skillExpansion_handleQuickCastModeInput

hookReplace combat_computePveDamage, skillExpansion_computePveDamage
hookReplace combat_resolvePveAttack, skillExpansion_resolvePveAttack
hookReplace combat_applyPveRangedHit, skillExpansion_applyPveRangedHit
hookReplace combat_resolveEnemyAttackOnPlayer, skillExpansion_resolveEnemyAttackOnPlayer
hookReplaceR3 combat_handleEnemyAttackOnPlayerStatusArmorSfx, skillExpansion_handleEnemyStatusArmor
hookReplace player_addHp, skillExpansion_addHp
hookReplace actor_player_tick, skillExpansion_playerTick

hookReplace scene_skillTree_exit, skillExpansion_treeExit
hookReplace scene_skillTree_handleDpad, skillExpansion_treeHandleDpad
hookReplace scene_skillTree_handleA, skillExpansion_treeHandleA
hookReplaceR3 scene_skillTree_drawPassiveTile, skillExpansion_treeDrawPassiveTile
hookReplaceR3 scene_skillTree_drawActiveTile, skillExpansion_treeDrawActiveTile
hookReplace scene_skillTree_drawDescription, skillExpansion_treeDrawDescription
hookReplace scene_skillTree_drawPassiveGrid, skillExpansion_treeDrawPassiveGrid
hookReplace scene_skillTree_getRequiredLevel, skillExpansion_treeGetRequiredLevel
hookReplace scene_skillTree_createQuickSelectMarkers, skillExpansion_treeCreateQuickSelectMarkers
hookReplace scene_skillTree_assignQuickSelect, skillExpansion_treeAssignQuickSelect

hookReplace player_updateHudSkillPalette, skillExpansion_hudUpdateSkillPalette
hookReplace player_tickHud, skillExpansion_tickHud
hookReplace input_update, skillExpansion_inputUpdate
hookReplace player_tickOverdrawCharge, skillExpansion_legolasOverdrawCharge

.if HACK_mod_save
    hookReplace scene_saveLoadMenu_loadSlot, skillExpansion_loadSlot
    hookReplace scene_saveLoadMenu_loadPeerSlot, skillExpansion_loadPeerSlot
.endif

; Cast state pose: new rows use their base row's pose instead of castingActiveSkill + 1.
; was: adds r0,r2,#1 ; adds r1,r7,#0 ; adds r1,#0x95 ; strb r0,[r1]
.org actor_player_tick + 0x17ac
    ldr r3, [pc, #0]
    bx  r3
    .word skill_expansion_cast_pose_hook|1

; Level-up grant: second skill point from level 21 on.
; was: ldrb r0,[r4] ; adds r0,#5 ; strb r0,[r4] ; ldrb r0,[r3] ; adds r0,#1 ; strb r0,[r3]
.org player_awardXp + 0x1fe
    ldr r0, [pc, #4]
    bx  r0
    nop
    .word skill_expansion_level_up_points_hook|1

; Burden carries between zones. The zone exit heal and the player actor spawn keep corruption.
; The spawn's following player_addCorruption(playerIndex, 0) re-applies its tier flags.
; was: strb r4,[r0] (Player.corruption = 0)
.org scene_gameplay_exit + 0x18c
    nop
; was: strb r2,[r0] (Player.corruption = 0)
.org actor_player_create + 0x196
    nop
