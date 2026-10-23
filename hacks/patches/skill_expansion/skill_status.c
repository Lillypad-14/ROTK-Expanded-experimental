#include "patches/skill_expansion/skill_status.h"
#include "patches/skill_expansion/skill_expansion_elite.h"

static const HackLibEffectDef Poison;
static const HackLibEffectDef Burn;
static const HackLibEffectDef Slow;

static void pulse(HackLibTimedEffect *effect)
{
    u32 damage = (u32)effect->magnitude * effect->stacks;
    skillExpansion_dealElementalDamage(
        effect->owner, effect->target, damage, HACKLIB_DAMAGE_PERIODIC, effect->abilityRow,
        effect->definition == &Poison ? HACKLIB_ELEMENT_POISON : HACKLIB_ELEMENT_FIRE);
}

static const HackLibEffectDef Poison = {
    .id = 0xb01, .interval = 30, .maxStacks = 3, .stackRule = HACKLIB_EFFECT_STACK, .pulse = pulse};
static const HackLibEffectDef Burn = {.id = 0xb02,
                                      .interval = 15,
                                      .maxStacks = 1,
                                      .stackRule = HACKLIB_EFFECT_REFRESH,
                                      .pulse = pulse};
static const HackLibEffectDef Slow = {
    .id = 0xb03, .maxStacks = 1, .stackRule = HACKLIB_EFFECT_REFRESH, .movementPercent = 50};

bool skillStatus_apply(u8 kind, Actor *owner, Actor *target, u16 frames, u16 power, u8 row)
{
    const HackLibEffectDef *definition = kind == SKILL_STATUS_POISON ? &Poison
                                         : kind == SKILL_STATUS_BURN ? &Burn
                                         : kind == SKILL_STATUS_SLOW ? &Slow
                                                                     : NULL;
    if (definition == NULL || frames == 0 || frames > 1800 || power > 21845 ||
        (kind != SKILL_STATUS_SLOW && power == 0) || !skillExpansion_isFoe(target))
    {
        return FALSE;
    }
    return hackLib_applyEffect(definition, owner, target, frames, power, row) != NULL;
}

u32 skillStatus_clear(Actor *target, u8 kind)
{
    const HackLibEffectDef *definition = kind == SKILL_STATUS_POISON ? &Poison
                                         : kind == SKILL_STATUS_BURN ? &Burn
                                         : kind == SKILL_STATUS_SLOW ? &Slow
                                                                     : NULL;
    u32 removed = 0;
    if (target == NULL || definition == NULL)
    {
        return 0;
    }
    for (u32 i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
    {
        const HackLibTimedEffect *effect = hackLib_effectAt(i);
        if (effect != NULL && effect->target == target && effect->definition == definition)
        {
            hackLib_removeEffect((HackLibTimedEffect *)effect);
            removed++;
        }
    }
    return removed;
}
