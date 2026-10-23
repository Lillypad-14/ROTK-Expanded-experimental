#pragma once

// Ringwraith: a ninth playable hero built from the Nazgul enemy sprites.

#include "patches/ringwraith/hero_registry.h"
#include "player.h"
#include "variables.h"

#define HERO_ID_RINGWRAITH 8
#define RINGWRAITH_HERO_COUNT HACK_HERO_COUNT

// Stock hero whose start items, loot, camera, spawn and menu art the Ringwraith uses.
#define RINGWRAITH_BASE_HERO HERO_ID_ARAGORN

// ObjPalettes index of the Nazgul palette.
#define RINGWRAITH_OBJ_PALETTE 6

// Animation bank layout: SpriteAnimation *[part][pose], one SpriteAnimation per facing variant.
#define RINGWRAITH_BANK_PART_COUNT 9
#define RINGWRAITH_BANK_POSE_COUNT 7

static inline bool32 ringwraith_isHero(u8 playerIndex)
{
    return PLAYER(playerIndex).heroId == HERO_ID_RINGWRAITH;
}

/** heroId, or the base hero for the Ringwraith, for stock per-hero tables of eight. */
static inline u32 ringwraith_stockHero(u32 heroId)
{
    const HeroDefinition *definition = ringwraith_getHeroDefinition(heroId);
    return definition != NULL ? definition->stockHero : RINGWRAITH_BASE_HERO;
}

char *ringwraith_getHeroName(u32 heroId);
