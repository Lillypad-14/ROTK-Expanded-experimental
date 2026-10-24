; Ringwraith: whole-function replacements, the nine-hero table repoints and the hero switch bounds.

.thumb

hookReplace player_initHeroStats, ringwraith_initHeroStats
hookReplace actor_player_updateHandSprites, ringwraith_updateHandSprites
hookReplace scene_saveLoadMenu_drawSlot, ringwraith_drawSlot
hookReplace scene_saveLoadMenu_refreshSlot, ringwraith_refreshSlot
.if HACK_skill_expansion
hookReplace player_addSpirit, ringwraith_addSpirit
hookReplace scene_gameplay_healAllPlayers, ringwraith_healAllPlayers
.endif

; Repoint one literal pool word, checking it still holds the stock table address.
.macro ringwraithRepoint, site, stock, table
    .if readu32("rotk.gba", site - 0x08000000) != stock
        .error "ringwraith: unexpected literal at " + tohex(site)
    .endif
    .org site
    .word table
.endmacro

.macro ringwraithRepointMove, site
    ringwraithRepoint site, HeroMoveAnimationBanks, RingwraithHeroMoveAnimationBanks
.endmacro
.macro ringwraithRepointMelee, site
    ringwraithRepoint site, HeroMeleeAnimationBanks, RingwraithHeroMeleeAnimationBanks
.endmacro
.macro ringwraithRepointCast, site
    ringwraithRepoint site, HeroCastAnimationBanks, RingwraithHeroCastAnimationBanks
.endmacro

ringwraithRepointMove 0x08025aec
ringwraithRepointMove 0x08025bbc
ringwraithRepointMove 0x08025bfc
ringwraithRepointMove 0x08025cb4
ringwraithRepointMove 0x0802740c
ringwraithRepointMove 0x08035b34
ringwraithRepointMove 0x0803601c
ringwraithRepointMove 0x08036170
ringwraithRepointMove 0x08036198
ringwraithRepointMove 0x080361c0
ringwraithRepointMove 0x080361f8
ringwraithRepointMove 0x08036220
ringwraithRepointMove 0x08036248
ringwraithRepointMove 0x08036324
ringwraithRepointMove 0x080363bc
ringwraithRepointMove 0x080363e0
ringwraithRepointMove 0x080363fc

ringwraithRepointMelee 0x08025d5c
ringwraithRepointMelee 0x08027418
ringwraithRepointMelee 0x08036270
ringwraithRepointMelee 0x08036298
ringwraithRepointMelee 0x080362d8
ringwraithRepointMelee 0x080363a0

ringwraithRepointCast 0x08027430
ringwraithRepointCast 0x0804478c
ringwraithRepointCast 0x08044858
ringwraithRepointCast 0x08044948
ringwraithRepointCast 0x08044a50
ringwraithRepointCast 0x08044bb4
ringwraithRepointCast 0x08044c7c
ringwraithRepointCast 0x08044d2c
ringwraithRepointCast 0x08044df4
ringwraithRepointCast 0x08044eb4
ringwraithRepointCast 0x08044f48
ringwraithRepointCast 0x0804500c

ringwraithRepoint 0x08027650, HeroPortraitBgAssets, RingwraithHeroPortraitBgAssets
ringwraithRepoint 0x080289cc, SkillTreeHeroTileBgAssets, RingwraithSkillTreeHeroTileBgAssets
ringwraithRepoint 0x0803ac40, LootHeroRegionProgressions, RingwraithLootHeroRegionProgressions
ringwraithRepoint 0x0803ac9c, LootHeroRegionProgressions, RingwraithLootHeroRegionProgressions
ringwraithRepoint 0x0803aed4, LootHeroItemTypeRolls, RingwraithLootHeroItemTypeRolls
ringwraithRepoint 0x0803130c, HeroStartMissions, RingwraithHeroStartMissions
ringwraithRepoint 0x08031318, HeroStartVariants, RingwraithHeroStartVariants

ringwraithRepoint 0x0801cb4c, RINGWRAITH_SPAWN_OFFSETS, RingwraithSpawnOffsets
ringwraithRepoint 0x08024340, RINGWRAITH_SPAWN_OFFSETS, RingwraithSpawnOffsets
ringwraithRepoint 0x08031ba8, RINGWRAITH_SPAWN_OFFSETS, RingwraithSpawnOffsets
ringwraithRepoint 0x0803edb4, RINGWRAITH_SPAWN_OFFSETS, RingwraithSpawnOffsets
ringwraithRepoint 0x08047728, RINGWRAITH_SPAWN_OFFSETS, RingwraithSpawnOffsets
ringwraithRepoint 0x0804a23c, RINGWRAITH_SPAWN_OFFSETS, RingwraithSpawnOffsets

ringwraithRepoint 0x0803e8cc, RINGWRAITH_CAMERA_HEIGHTS, RingwraithCameraHeights
ringwraithRepoint 0x0803e974, RINGWRAITH_CAMERA_HEIGHTS, RingwraithCameraHeights
ringwraithRepoint 0x0803eb94, RINGWRAITH_CAMERA_HEIGHTS, RingwraithCameraHeights

; Start items and backpack art: nine-case switches.
; was: cmp r0, #7
.org player_initItemsAndHollow + 0x1e
    cmp  r0, #HACK_HERO_LAST
ringwraithRepoint player_initItemsAndHollow + 0x38, RINGWRAITH_START_ITEMS_CASES, RingwraithStartItemsCases
.org scene_backpack_enter + 0x6c
    cmp  r0, #HACK_HERO_LAST
ringwraithRepoint scene_backpack_enter + 0x98, RINGWRAITH_BACKPACK_CASES, RingwraithBackpackCases

; Player palette: Smeagol 0x2a, the Ringwraith the Nazgul palette, others heroId + 0x21.
; was: cmp r5,#7 ; bne ; adds r0,r5,#2 ; lsls r0,#24
.org actor_applyTypeDefaults + 0xb0
    ldr  r0, [pc, #0]
    bx   r0
    .word ringwraith_palette_hook|1

; Skill tree title. was: ldrb r0,[r0] ; adds r0,#19 ; bl text_getString
.org scene_skillTree_enter + 0xec
    ldr  r2, [pc, #0]
    bx   r2
    .word ringwraith_skill_tree_name_hook|1

; Character stats screen hero name. was: ldrb r0,[r0] ; adds r0,#19 ; bl text_getString
.if readu16("rotk.gba", 0x08027bde - 0x08000000) != 0x3013
    .error "stats screen name: unexpected native instructions"
.endif
.org 0x08027bdc
    ldr  r2, [pc, #0]
    bx   r2
    .word ringwraith_stats_name_hook|1

; Equipment compatibility aliases from the character registry.
.macro heroEquipmentHook, site, registerNumber, handler
    .if readu16("rotk.gba", site - 0x08000000) != (0x2806 | (registerNumber << 8)) || readu16("rotk.gba", site - 0x08000000 + 2) != 0xd100 || readu16("rotk.gba", site - 0x08000000 + 4) != (0x2000 | (registerNumber << 8))
        .error "hero equipment alias: unexpected native instructions"
    .endif
    .org site
        bl handler
        nop
.endmacro

heroEquipmentHook item_getEquipSlot + 0x3e, 3, ringwraith_equipment_r3
heroEquipmentHook item_applyAffixStats + 0x6c, 2, ringwraith_equipment_r2
heroEquipmentHook player_getMaxSpiritBonus + 0x246, 3, ringwraith_equipment_r3
heroEquipmentHook item_isUsableByPlayer + 0x28, 3, ringwraith_equipment_r3

; Reuse the replaced slot renderer's body for nearby equipment lookup veneers.
.org scene_saveLoadMenu_drawSlot + 8

.align 2
ringwraith_equipment_r3:
    push {r0}
    ldr r0, =HeroEquipmentIds
    ldrb r3, [r0, r3]
    pop {r0}
    bx lr
    .pool
.align 2
ringwraith_equipment_r2:
    push {r0}
    ldr r0, =HeroEquipmentIds
    ldrb r2, [r0, r2]
    pop {r0}
    bx lr
    .pool
