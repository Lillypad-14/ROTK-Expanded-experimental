# Ringwraith as a 9th playable hero (HeroId 8) - scoping notes

Decided 2026-10-20: straight to a 9th hero (no swap prototype). Claude drafts the skill kit, user approves.

## Assets available
- Enemy only: EnemyTypeNazgul banks Walk / StandCopy / Death / Stagger / Attack, 5 facings each
  (carved/config/enemy_graphics.cfg 0x080546b0..). Single sprite, sword baked in.
- EnemyTypeFlyingNazgul Walk / Death / Attack, facing 0 only (fell beast).
- No portrait, select-screen sprite, skill-tree tile bg, dialog portrait.

## Hero animation model (to RE)
- Player bodies are multi-sprite: clip = animationBank[slotPartIds[slot] * 7 + pose] (actor.h SpriteRender).
- Banks per hero: HeroMoveAnimationBanks / HeroMeleeAnimationBanks / HeroCastAnimationBanks (void*[8], untyped).
- Hand sprites (actor_player_updateHandSprites) must be skipped for the wraith.
- Plan: build three synthetic banks in modcode rodata that map every pose to Nazgul clips
  (move: walk/stand, melee: attack, cast: attack or stand, hurt: stagger, death: death).

## Hero-indexed tables to extend (config/data.cfg, all [8])
HeroStartMissions, HeroStartVariants, LootHeroItemTypeRolls, HeroMove/Melee/CastAnimationBanks,
HeroPortraitBgAssets, SkillTreeHeroTileBgAssets, SaveLoadHeroAnimationPtrs,
SaveLoadHeroSelectedAnimationPtrs, SaveLoadHeroPalettes, LootHeroRegionProgressions,
ActiveSkillDescriptionBaseTextIds, ActiveSkillRequiredLevels[8][6][6], per-hero ActiveSkills/PassiveSkillIds blocks.
Each needs: relocate to modcode (9 entries) + repoint every ROM xref, or a hook that special-cases heroId 8.
Also: per-hero switches in code, NPC variant 0..7 mirror (actor/npc.h), save heroId, unlock flags,
hero-select scene (not yet located; scene_saveLoadMenu_createCharacter 0x08030ff4 is a lead).

## Status (2026-10-20): implemented, untested in mGBA
- Plumbing: hacks/patches/ringwraith/{ringwraith.c,ringwraith.asm,ringwraith_hook.asm}.
- Kit: hacks/patches/ringwraith/ringwraith_kit.c (RingwraithHeroKit via skillExpansion_getHeroKit).
  Dread table (24 marks, keyed by Actor*), player_addHp/addSpirit whole-function wraps,
  hit/tick/reset hooks called from skill_expansion_elite.c under HACK_ringwraith.
- Deviations from approved kit: full Dread stuns (no flee), Fell Beast has no loot bonus, no Ring
  meter, Black Breath fixed at level 1 (Herbal Healing row rules).
- Open questions to check in mGBA: thrall count decrement on ghost despawn, potions consumed with
  no heal, cast poses, start-spirit clamp after mission load, Unseen stealth visuals.

## Wraith gear (2026-10-20, built, untested in game)

`hacks/patches/ringwraith/wraith_gear.c/.h/.asm` + `wraith_gear_tables.asm` (post-modcode overlay in hack.asm).
- Wraith* tables = stock rows (incbin'd from rotk.gba) + wraith tail; ItemTypeInfos types 1,4,8,10,11,12,13 repointed. baseItemCount stays stock so stock rolls never pick wraith rows.
- loot_generateItem replaced: wraith rolls wraith gear only; others = stock C copy.
- Names: text IDs 0x4000+ via text_overrides findString. Uniques orange via free BG palette bank scan (falls back to red).
- item_getEquipSlot: non-wraith heroes can't equip wraith gear. item_descriptions shows wraith effects + unique text.
- Effects summed per tick into wraithGear_stats; consumed in ringwraith_kit.c.
- Untested risks: orange bank detection in gameplay HUD, description length in the info panel.
