#pragma once

#include "lib/timed_effect.h"

enum SkillStatusKind
{
    SKILL_STATUS_NONE = 0,
    SKILL_STATUS_POISON = 1,
    SKILL_STATUS_BURN = 2,
    SKILL_STATUS_SLOW = 3,
};

// Poison stacks to three per owner. Burn and slow refresh. Slow halves tick displacement.
bool skillStatus_apply(u8 kind, Actor *owner, Actor *target, u16 frames, u16 power, u8 row);

// Remove one shared status kind from every caster on this target.
u32 skillStatus_clear(Actor *target, u8 kind);
