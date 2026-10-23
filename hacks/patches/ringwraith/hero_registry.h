#pragma once

#include "hero_count.h"
#include "player.h"
#include "spriteAnimation.h"

typedef struct HeroDefinition
{
    u8 stockHero;
    u8 equipmentHero;
    u8 palette;
    bool detachHands;
    const char *name;
    const HeroBaseStats *stats;
    const SpriteAnimation *menuIdle;
    const SpriteAnimation *menuSelected;
} HeroDefinition;

extern const HeroBaseStats RingwraithBaseStats;
extern const HeroDefinition HeroDefinitions[HACK_HERO_COUNT];
const HeroDefinition *ringwraith_getHeroDefinition(u32 heroId);
