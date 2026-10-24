#include "patches/ringwraith/ringwraith.h"
#include "patches/ringwraith/ringwraith_kit.h"
#include "font.h"
#include "gfx.h"
#include "mission.h"
#include "save.h"
#include "scene/saveLoadMenu.h"
#include "sprite.h"
#include "spriteAnimation.h"
#include "text.h"
#include "variables.h"

#define NAZGUL_WALK EnemyAnimationBank_EnemyTypeNazgul_Walk
#define NAZGUL_STAND EnemyAnimationBank_EnemyTypeNazgul_StandCopy
#define NAZGUL_DEATH EnemyAnimationBank_EnemyTypeNazgul_Death
#define NAZGUL_STAGGER EnemyAnimationBank_EnemyTypeNazgul_Stagger
#define NAZGUL_ATTACK EnemyAnimationBank_EnemyTypeNazgul_Attack

// The Nazgul is one sprite, so every part shows the same clips. The hand slots stay detached.
#define BANK(p0, p1, p2, p3, p4, p5, p6)                                                           \
    p0, p1, p2, p3, p4, p5, p6, p0, p1, p2, p3, p4, p5, p6, p0, p1, p2, p3, p4, p5, p6, p0, p1,    \
        p2, p3, p4, p5, p6, p0, p1, p2, p3, p4, p5, p6, p0, p1, p2, p3, p4, p5, p6, p0, p1, p2,    \
        p3, p4, p5, p6, p0, p1, p2, p3, p4, p5, p6, p0, p1, p2, p3, p4, p5, p6

#define BANK_SIZE (RINGWRAITH_BANK_PART_COUNT * RINGWRAITH_BANK_POSE_COUNT)

// Poses: knocked down, death, getting up, hurt, idle, knocked back, walk.
const SpriteAnimation *const RingwraithMoveAnimationBank[BANK_SIZE] = {
    BANK(NAZGUL_STAGGER, NAZGUL_DEATH, NAZGUL_STAGGER, NAZGUL_STAGGER, NAZGUL_STAND, NAZGUL_STAGGER,
         NAZGUL_WALK),
};

const SpriteAnimation *const RingwraithMeleeAnimationBank[BANK_SIZE] = {
    BANK(NAZGUL_ATTACK, NAZGUL_ATTACK, NAZGUL_ATTACK, NAZGUL_ATTACK, NAZGUL_ATTACK, NAZGUL_ATTACK,
         NAZGUL_ATTACK),
};

// Cast pose is active row + 1. Morgul Blade (row 0) is a stab, the rest are cries and gestures.
const SpriteAnimation *const RingwraithCastAnimationBank[BANK_SIZE] = {
    BANK(NAZGUL_STAGGER, NAZGUL_ATTACK, NAZGUL_STAGGER, NAZGUL_STAGGER, NAZGUL_STAGGER,
         NAZGUL_STAGGER, NAZGUL_STAGGER),
};

const HeroBaseStats RingwraithBaseStats = {
    .courage = 0x1c,
    .defense = 0x18,
    .health = 0x18,
    .accuracy = 0x14,
    .strength = 0x1b,
};

// Each original runs the 8-byte prologue that its hookReplace overwrote, then continues past it.
asm(".section .text\n"
    ".balign 4\n"
    ".thumb\n"
    ".global ringwraith_original_player_initHeroStats\n"
    ".thumb_func\n"
    "ringwraith_original_player_initHeroStats:\n"
    "    push {r4, lr}\n"
    "    lsl  r0, r0, #24\n"
    "    lsr  r0, r0, #24\n"
    "    lsl  r1, r1, #24\n"
    "    ldr  r3, =player_initHeroStats + 8\n"
    "    bx   r3\n"
    "    .pool\n"
    ".balign 4\n"
    ".global ringwraith_original_actor_player_updateHandSprites\n"
    ".thumb_func\n"
    "ringwraith_original_actor_player_updateHandSprites:\n"
    "    push {r4-r7, lr}\n"
    "    mov  r7, r10\n"
    "    mov  r6, r9\n"
    "    mov  r5, r8\n"
    "    ldr  r3, =actor_player_updateHandSprites + 8\n"
    "    bx   r3\n"
    "    .pool\n"
    ".balign 4\n"
    ".global ringwraith_original_scene_gameplay_healAllPlayers\n"
    ".thumb_func\n"
    "ringwraith_original_scene_gameplay_healAllPlayers:\n"
    "    push {r4-r7, lr}\n"
    "    mov  r7, r9\n"
    "    mov  r6, r8\n"
    "    push {r6, r7}\n"
    "    ldr  r3, =scene_gameplay_healAllPlayers + 8\n"
    "    bx   r3\n"
    "    .pool\n");

void ringwraith_original_player_initHeroStats(u32 heroId, u32 playerIndex);
void ringwraith_original_actor_player_updateHandSprites(Actor *actor);
void ringwraith_original_scene_gameplay_healAllPlayers(void);

/**
 * The wraith's own rules block healing from outside its drains, so it is told the game's
 * full heal at a save load or menu launch is allowed. Burden is not reset on a zone change, so
 * a load clears it here instead of carrying it over from the previous session.
 *
 * REPLACES (whole function): scene_gameplay_healAllPlayers.
 */
void ringwraith_healAllPlayers(void)
{
    u8 p;

    ringwraith_kitSetFullHeal(TRUE);
    ringwraith_original_scene_gameplay_healAllPlayers();
    ringwraith_kitSetFullHeal(FALSE);
    for (p = 0; p < 2; p++)
    {
        PLAYER(p).corruption = 0;
        player_addCorruption(p, 0);
    }
}

/**
 * The stock switch skips unknown heroes, so the Ringwraith's stats are written first.
 *
 * REPLACES (whole function): player_initHeroStats.
 */
void ringwraith_initHeroStats(u32 heroId, u32 playerIndex)
{
    u8 p = playerIndex;
    const HeroDefinition *definition = ringwraith_getHeroDefinition(heroId & 0xff);

    if (definition != NULL && definition->stats != NULL)
    {
        u16 maxHp = PLAYER(p).baseStats.maxHp;

        PLAYER(p).baseStats = *definition->stats;
        PLAYER(p).baseStats.maxHp = maxHp;
    }
    ringwraith_original_player_initHeroStats(heroId, playerIndex);
}

/**
 * The Ringwraith's blade is part of its sprite, so both hand slots stay detached.
 *
 * REPLACES (whole function): actor_player_updateHandSprites.
 */
void ringwraith_updateHandSprites(Actor *actor)
{
    u8 p = actor->as.combat.playerIndex;
    const HeroDefinition *definition = ringwraith_getHeroDefinition(PLAYER(p).heroId);

    if (definition == NULL || !definition->detachHands)
    {
        ringwraith_original_actor_player_updateHandSprites(actor);
        return;
    }
    sprite_detachSlot(actor, 0);
    sprite_clearSlotFlag1(actor, 0);
    player_clearHandFx(p, 0);
    sprite_detachSlot(actor, 1);
    player_clearHandFx(p, 1);
    PLAYER(p).combatFlags.p &= ~PLAYER_COMBAT_FLAG_HAND_SPRITES_DIRTY;
}

char *ringwraith_getHeroName(u32 heroId)
{
    const HeroDefinition *definition = ringwraith_getHeroDefinition(heroId);
    if (definition != NULL && definition->name != NULL)
    {
        return (char *)definition->name;
    }
    return text_getString(TEXT_ID_HERO_NAME_BASE + ringwraith_stockHero(heroId));
}

/**
 * Stock body with the hero names from ringwraith_getHeroName.
 *
 * REPLACES (whole function): scene_saveLoadMenu_drawSlot.
 */
void ringwraith_drawSlot(u8 slot, u8 selectedSlot)
{
    char levelText[16];
    s32 color;
    s32 tile;
    u32 x;
    u32 y;
    SaveSlot *save;
    u8 mission;
    char *str;

    color = slot == selectedSlot ? 6 : 12;
    font_setStyle(FONT_STYLE_8PX_A, color, -1);
    tile = slot * 60 + 0x5b;
    x = (slot & 1) ? 0xac : 0x44;
    y = slot > 1 ? 0x53 : 0x1b;
    gfx_fillBgRect(1, (x >> 3) - 4, y >> 3, 10, 5, 0);
    save = &g_SaveSlots[slot];
    mission = save_getSlotMission(save);
    if (mission < SLOT_MISSION_EMPTY)
    {
        tile = font_drawString(tile, x, y, ringwraith_getHeroName(save->d.hero.d.heroId), 1);
        y += 8;
        str = text_getString(TEXT_ID_REGION_NAME_BASE + mission_getRegion(mission));
        tile = font_drawString(tile, x, y, str, 1);
        y += 8;
        str = text_getString(TEXT_ID_MISSION_NAME_BASE + mission);
        tile = font_drawString(tile, x, y, str, 1);
        y += 8;
        font_setTemplateArg0(text_formatInt(save->d.level + 1, levelText));
        font_drawString(tile, x, y, text_getString(TEXT_ID_LEVEL_1), 1);
    }
    else if (SCENE_ARG_SAVE_LOAD_STATE == SAVE_LOAD_STATE_HERO_SELECT)
    {
        tile = font_drawString(tile, x, y, text_getString(TEXT_ID_SELECT_HERO), 1);
        y += 8;
        font_drawString(tile, x, y, ringwraith_getHeroName(SCENE_ARG_SAVE_LOAD_CURSOR), 1);
    }
    else
    {
        font_drawString(tile, x, y, text_getString(TEXT_ID_EMPTY), 1);
    }
}

#define SLOT_HERO_NONE 0xff

/**
 * Stock body with the Ringwraith's still and walking Nazgul sprites and the Nazgul palette.
 *
 * REPLACES (whole function): scene_saveLoadMenu_refreshSlot.
 */
void ringwraith_refreshSlot(s32 slotIndex)
{
    Actor *obj = g_SaveLoadSlotObjs[slotIndex];
    u32 heroId = g_SaveSlots[slotIndex].d.hero.d.heroId;
    bool32 selected = slotIndex == (s32)SCENE_ARG_SAVE_LOAD_SELECTED_SLOT;

    if (save_getSlotMission(&g_SaveSlots[slotIndex]) < SLOT_MISSION_EMPTY)
    {
        obj->flags.p |= ACTOR_FLAG_RENDER;
    }
    else if (SCENE_ARG_SAVE_LOAD_STATE == SAVE_LOAD_STATE_HERO_SELECT)
    {
        obj->flags.p |= ACTOR_FLAG_RENDER;
        heroId = SCENE_ARG_SAVE_LOAD_CURSOR;
    }
    else
    {
        obj->flags.p &= ~ACTOR_FLAG_RENDER;
        heroId = SLOT_HERO_NONE;
    }
    const HeroDefinition *definition = ringwraith_getHeroDefinition(heroId);
    if (definition != NULL && definition->menuIdle != NULL && definition->menuSelected != NULL)
    {
        sprite_setAnimation(obj, selected ? definition->menuSelected : definition->menuIdle);
        gfx_queuePaletteWrite(ObjPalettes[definition->palette], slotIndex << 4, 16);
    }
    else if (heroId != SLOT_HERO_NONE)
    {
        heroId = ringwraith_stockHero(heroId);
        sprite_setAnimation(obj, selected ? SaveLoadHeroSelectedAnimationPtrs[heroId]
                                          : SaveLoadHeroAnimationPtrs[heroId]);
        gfx_queuePaletteWrite(SaveLoadHeroPalettes[heroId], slotIndex << 4, 16);
    }
}
