#include "lib/skill_fx.h"
#include "patches/skill_expansion/skill_delivery.h"
#include "patches/skill_expansion/skill_expansion.h"
#include <stddef.h>

_Static_assert(SKILL_EXPANSION_HERO_COUNT <= 16, "hero IDs exceed the save field");
_Static_assert(SKILL_EXPANSION_ACTIVE_ROW_COUNT <= 256, "quick select rows exceed the save field");
_Static_assert(SKILL_EXPANSION_PASSIVE_END <= 255, "passive IDs exceed the empty-slot sentinel");
_Static_assert(ACTIVE_SKILL_MAX_LEVEL <= 7, "stock purchased levels exceed the save field");
_Static_assert(ARRAY_COUNT(((ActiveSkillValue *)0)->perLevel) == ACTIVE_SKILL_MAX_LEVEL,
               "active value curve length");

// Build-tool schema of compiler-derived sizes and member positions.
const u32 SkillExpansionValidationSchema[] = {
    SKILL_EXPANSION_HERO_COUNT,
    HERO_ACTIVE_SKILL_COUNT,
    SKILL_EXPANSION_NEW_ACTIVE_COUNT,
    SKILL_EXPANSION_REQUIRED_LEVEL_COUNT,
    SKILL_EXPANSION_PASSIVE_END,
    sizeof(SkillExpansionHeroKit),
    offsetof(SkillExpansionHeroKit, activeSkills),
    offsetof(SkillExpansionHeroKit, icons),
    offsetof(SkillExpansionHeroKit, requiredLevels),
    offsetof(SkillExpansionHeroKit, passiveSkillIds),
    offsetof(SkillExpansionHeroKit, newActives),
    offsetof(SkillExpansionHeroKit, newPassiveSkillIds),
    offsetof(SkillExpansionHeroKit, resolvers),
    sizeof(ActiveSkill),
    offsetof(ActiveSkill, callback),
    sizeof(SkillExpansionActiveDef),
    offsetof(SkillExpansionActiveDef, row),
    offsetof(SkillExpansionActiveDef, stockCallback),
    offsetof(SkillExpansionActiveDef, icon),
    offsetof(SkillExpansionActiveDef, requiredLevels),
    offsetof(SkillExpansionActiveDef, resolve),
    offsetof(SkillExpansionActiveDef, baseRow),
    HERO_PASSIVE_SKILL_COUNT,
    SKILL_EXPANSION_NEW_PASSIVE_COUNT,
    offsetof(SkillExpansionActiveDef, policy),
    offsetof(SkillExpansionActiveDef, id),
    offsetof(SkillExpansionHeroKit, policies),
    offsetof(SkillExpansionHeroKit, activeIds),
    sizeof(SkillExpansionCastPolicy),
    offsetof(SkillExpansionCastPolicy, cooldownFrames),
    offsetof(SkillExpansionHeroKit, artHeroId),
    offsetof(SkillExpansionActiveDef, timeline),
    offsetof(SkillExpansionHeroKit, timelines),
    sizeof(HackLibCastTimeline),
    offsetof(HackLibCastTimeline, releaseTick),
    offsetof(HackLibCastTimeline, recoveryTicks),
    offsetof(HackLibCastTimeline, count),
    sizeof(HackLibTimelineEvent),
    offsetof(HackLibTimelineEvent, effect),
    offsetof(HackLibTimelineEvent, anchor),
    HACKLIB_VISUAL_EFFECT_COUNT,
    offsetof(SkillExpansionActiveDef, delivery),
    sizeof(SkillDeliveryDef),
};
