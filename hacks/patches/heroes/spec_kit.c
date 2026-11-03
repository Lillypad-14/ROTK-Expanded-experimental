// The skill engine of data-written kits (spec_kit.h). Every hero with a spec shares these
// callbacks; its own tables say what each row does. hero_kit.py links those tables apart from the
// modcode into the hero's kit slot (SKILL_EXPANSION_KIT_SLOT), so this code never names them.
#include "patches/heroes/spec_kit.h"
#include "hero_count.h"
#include "lib/fx_kit.h"
#include "math.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "player.h"
#include "stats.h"
#include "variables.h"

#if PC_EDITION && HACK_skill_expansion && HACK_ringwraith

#define POLICY(frames) {1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, frames}
const SkillExpansionCastPolicy SpecKitQuickPolicy = POLICY(12);
const SkillExpansionCastPolicy SpecKitNormalPolicy = POLICY(30);
const SkillExpansionCastPolicy SpecKitLongPolicy = POLICY(90);
const SkillExpansionCastPolicy SpecKitUltimatePolicy = POLICY(180);


typedef struct SpecState
{
    u16 wardFrames;
    u16 empowerFrames;
    u8 wardPercent;
    u8 empowerPercent;
} SpecState;

_Static_assert(sizeof(SpecState) <= sizeof(skillExpansion_heroState[0]), "SpecState exceeds the hero state");

/** The player's spec rows (SpecSkill *[13]), or NULL for a hero without a spec kit. */
static const SpecSkill *const *specRows(u8 playerIndex)
{
    u8 hero = PLAYER(playerIndex).heroId;

    return hero < SKILL_EXPANSION_KIT_SLOTS ? SKILL_EXPANSION_KIT_SLOT(hero)->specRows : NULL;
}

/** The player's spec state; call only for a spec hero. */
static SpecState *state(u8 playerIndex)
{
    return (SpecState *)skillExpansion_heroStateClaim(playerIndex, PLAYER(playerIndex).heroId,
                                                      PLAYER(playerIndex).heroId);
}

static bool32 launch(Actor *actor, u8 playerIndex, const SpecSkill *skill, u8 level, u8 row)
{
    Vector2Fp16 at = actor->coords;
    u8 direction = actor->facing < 8 ? actor->facing * 4 : 0;

    if (skill->targetRange != 0)
    {
        Actor *target = skillExpansion_findTarget(actor, skill->targetRange, NULL, 0);
        Vector2Fp16 toward;

        if (target == NULL)
        {
            return FALSE;
        }
        at = target->coords;
        toward.x = target->coords.x - actor->coords.x;
        toward.y = target->coords.y - actor->coords.y;
        direction = math_vector2_direction32(toward);
    }
    return skillDelivery_launch(playerIndex, skill->delivery, &at, direction, level, row, FALSE);
}

SkillExpansionCastResult specKit_resolve(Actor *actor)
{
    u8 player = actor->as.combat.playerIndex;
    u8 row = PLAYER(player).castingActiveSkill;
    const SpecSkill *const *rows = specRows(player);
    u32 level = skillExpansion_effectiveActiveLevel(player, row);
    const SpecSkill *skill;
    SpecState *st;

    if (rows == NULL || row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT || rows[row] == NULL || level == 0)
    {
        return SKILL_CAST_UNAVAILABLE;
    }
    skill = rows[row];
    level = level > 5 ? 5 : level;
    st = state(player);
    switch (skill->effect)
    {
    case SPEC_EFFECT_DELIVERY:
        if (!launch(actor, player, skill, (u8)level, row))
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        break;
    case SPEC_EFFECT_HEAL:
        if (PLAYER(player).currentHp >= PLAYER_STAT(player, STAT_MAX_HP))
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        player_addHp(player, (s16)skill->amount[level - 1], FALSE);
        break;
    case SPEC_EFFECT_WARD:
        st->wardPercent = (u8)skill->amount[level - 1];
        st->wardFrames = skill->frames;
        break;
    case SPEC_EFFECT_EMPOWER:
        st->empowerPercent = (u8)skill->amount[level - 1];
        st->empowerFrames = skill->frames;
        break;
    default:
        return SKILL_CAST_UNAVAILABLE;
    }
    if (skill->fxColor != 0xff)
    {
        hackLib_fxBurst(actor, skill->fxShape, skill->fxColor, FX_LAYER_FRONT);
    }
    return SKILL_CAST_SUCCESS;
}

void specKit_reset(u8 playerIndex)
{
    if (specRows(playerIndex) != NULL)
    {
        SpecState *st = state(playerIndex);

        st->wardFrames = st->empowerFrames = 0;
    }
}

void specKit_cleanup(u8 playerIndex, Actor *actor)
{
    (void)actor;
    specKit_reset(playerIndex);
}

void specKit_tick(u8 playerIndex, Actor *actor)
{
    SpecState *st;

    (void)actor;
    if (specRows(playerIndex) == NULL)
    {
        return;
    }
    st = state(playerIndex);
    if (st->wardFrames != 0)
    {
        st->wardFrames--;
    }
    if (st->empowerFrames != 0)
    {
        st->empowerFrames--;
    }
}

s32 specKit_meleeDamage(u8 playerIndex, Actor *victim, s32 damage)
{
    SpecState *st;

    (void)victim;
    if (specRows(playerIndex) == NULL)
    {
        return damage;
    }
    st = state(playerIndex);
    return st->empowerFrames != 0 ? damage + damage * st->empowerPercent / 100 : damage;
}

s32 specKit_incomingDamage(u8 playerIndex, Actor *attacker, s32 damage)
{
    SpecState *st;

    (void)attacker;
    if (damage <= 0 || specRows(playerIndex) == NULL)
    {
        return damage;
    }
    st = state(playerIndex);
    if (st->wardFrames != 0)
    {
        damage -= damage * st->wardPercent / 100;
        if (damage < 1)
        {
            damage = 1;
        }
    }
    return damage;
}

#endif
