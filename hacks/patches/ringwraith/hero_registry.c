#include "patches/ringwraith/hero_registry.h"
#include "hero_stat_declarations.h"
#include "variables.h"

const HeroDefinition HeroDefinitions[HACK_HERO_COUNT] = {
#include "hero_definitions.inc"
};

const HeroDefinition *ringwraith_getHeroDefinition(u32 heroId)
{
    return heroId < HACK_HERO_COUNT ? &HeroDefinitions[heroId] : NULL;
}
