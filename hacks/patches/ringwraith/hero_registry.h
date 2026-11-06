#pragma once

#include "hero_count.h"
#include "player.h"
#include "spriteAnimation.h"

typedef struct HeroDefinition
{
    u8 stockHero;
    u8 equipmentHero;
    u8 lootHero;
    u8 palette;
    bool detachHands;
    const char *name;
    const HeroBaseStats *stats;
    const SpriteAnimation *menuIdle;
    const SpriteAnimation *menuSelected;
    // Name for narrow places such as the save slot card, or NULL to use name.
    const char *shortName;
} HeroDefinition;

extern const HeroBaseStats RingwraithBaseStats;
extern const HeroDefinition HeroDefinitions[HACK_HERO_COUNT];
const HeroDefinition *ringwraith_getHeroDefinition(u32 heroId);
