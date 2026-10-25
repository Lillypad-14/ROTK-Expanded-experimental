#include "lib/targeting.h"
#include "lib/actor_ref.h"
#include "lib/profile.h"
#include "variables.h"
#if HACK_gauntlet
#include "lib/combat_source.h"
#include "patches/gauntlet/gauntlet_boon_effects.h"
#endif

bool hackLib_isLiveActor(const Actor *actor)
{
    return hackLib_isActorRefValid(hackLib_actorRef((Actor *)actor));
}

static bool matches(const HackLibTargetQuery *query, Actor *target)
{
    s32 dx = (target->coords.x - query->center->coords.x) >> FP16_16_SHIFT;
    s32 dy = (target->coords.y - query->center->coords.y) >> FP16_16_SHIFT;
    s32 directionLength = query->facing.x * query->facing.x + query->facing.y * query->facing.y;
    s32 dot = dx * query->facing.x + dy * query->facing.y;
    u32 distance;

    if (target == query->exclude || target == query->center ||
        hackLib_actorCurrentHp(target) <= 0 || (target->flags.p & ACTOR_FLAG_PENDING_REMOVE) ||
        ABS(dx) > query->radius || ABS(dy) > query->radius)
    {
        return FALSE;
    }
    distance = (u32)(dx * dx) + (u32)(dy * dy);
    if (distance > (u32)query->radius * query->radius)
    {
        return FALSE;
    }
    if (directionLength != 0 && distance != 0 &&
        (dot < 0 || (u64)dot * dot * 65536 <
                        (u64)distance * directionLength * query->coneCosine * query->coneCosine))
    {
        return FALSE;
    }
    return query->filter == NULL || query->filter(target, query->argument);
}

Actor *hackLib_nextTarget(const HackLibTargetQuery *query, Actor *cursor)
{
    if (query == NULL || query->center == NULL || query->radius > 32767 ||
        ABS(query->facing.x) > 1 || ABS(query->facing.y) > 1 || query->coneCosine > 256)
    {
        return NULL;
    }
#if HACK_gauntlet
    HackLibTargetQuery widened = *query;
    const HackLibDamageSource *source = hackLib_currentDamageSource();
    if (source != NULL && source->abilityRow != HACKLIB_ABILITY_NONE)
    {
        widened.radius = gauntletBoons_radius(source->origin, query->radius);
        query = &widened;
    }
#endif
    HackLibProfileSpan profile = hackLib_profileBegin(HACKLIB_PROFILE_TARGETS);
    cursor = cursor != NULL ? cursor->next : g_ActorListMain.head;
    for (; cursor != NULL; cursor = cursor->next)
    {
        hackLib_profileWork(HACKLIB_PROFILE_TARGETS, 1);
        if (matches(query, cursor))
        {
            hackLib_profileEnd(profile);
            return cursor;
        }
    }
    hackLib_profileEnd(profile);
    return NULL;
}

Actor *hackLib_nearestTarget(const HackLibTargetQuery *query)
{
    Actor *cursor = NULL;
    Actor *best = NULL;
    u32 bestDistance = 0;

    while ((cursor = hackLib_nextTarget(query, cursor)) != NULL)
    {
        s32 dx = (cursor->coords.x - query->center->coords.x) >> FP16_16_SHIFT;
        s32 dy = (cursor->coords.y - query->center->coords.y) >> FP16_16_SHIFT;
        u32 distance = (u32)(dx * dx) + (u32)(dy * dy);

        if (best == NULL || distance < bestDistance)
        {
            best = cursor;
            bestDistance = distance;
        }
    }
    return best;
}

Actor *hackLib_nextTargetAt(const Vector2Fp16 *position, Actor *exclude, u16 radius, Actor *cursor)
{
    if (position == NULL || radius > 32767)
    {
        return NULL;
    }
#if HACK_gauntlet
    const HackLibDamageSource *source = hackLib_currentDamageSource();
    if (source != NULL && source->abilityRow != HACKLIB_ABILITY_NONE)
    {
        radius = gauntletBoons_radius(source->origin, radius);
    }
#endif
    HackLibProfileSpan profile = hackLib_profileBegin(HACKLIB_PROFILE_TARGETS);
    cursor = cursor != NULL ? cursor->next : g_ActorListMain.head;
    for (; cursor != NULL; cursor = cursor->next)
    {
        hackLib_profileWork(HACKLIB_PROFILE_TARGETS, 1);
        s32 dx = (cursor->coords.x >> FP16_16_SHIFT) - (position->x >> FP16_16_SHIFT);
        s32 dy = (cursor->coords.y >> FP16_16_SHIFT) - (position->y >> FP16_16_SHIFT);
        if (cursor != exclude && !(cursor->flags.p & ACTOR_FLAG_PENDING_REMOVE) &&
            hackLib_actorCurrentHp(cursor) > 0 && ABS(dx) <= radius && ABS(dy) <= radius &&
            (u32)(dx * dx) + (u32)(dy * dy) <= (u32)radius * radius)
        {
            hackLib_profileEnd(profile);
            return cursor;
        }
    }
    hackLib_profileEnd(profile);
    return NULL;
}
