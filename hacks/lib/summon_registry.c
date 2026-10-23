#include "lib/summon_registry.h"
#include "lib/actor_ref.h"
#include "lib/combat_source.h"
#include "lib/profile.h"
#include "lib/timed_effect.h"
#include "memory.h"

static HackLibSummon *summons;
static u8 tickDepth;

static void tickSummon(Actor *actor);

static bool isAttached(const HackLibSummon *summon)
{
    return hackLib_isActorRefValid((HackLibActorRef){summon->actor, summon->actorGeneration}) &&
           hackLib_actorHasTick(summon->actor, tickSummon) &&
           summon->actor->parentActor == summon->owner;
}

static void release(HackLibSummon *summon, bool despawn)
{
    if (isAttached(summon))
    {
        hackLib_replaceActorTick(summon->actor, tickSummon, summon->originalTick);
        if (despawn)
        {
            summon->definition->despawn(summon->actor);
        }
    }
    memory_memset(summon, 0, sizeof(*summon));
}

static void reapSummons(void)
{
    u32 i;
    bool any = FALSE;

    if (summons == NULL)
    {
        return;
    }

    for (i = 0; i < HACKLIB_SUMMON_LIMIT; i++)
    {
        HackLibSummon *summon = &summons[i];
        if (summon->definition != NULL && !isAttached(summon))
        {
            release(summon, FALSE);
        }
        else if (summon->definition != NULL && !hackLib_isActorRefValid((HackLibActorRef){
                                                   summon->owner, summon->ownerGeneration}))
        {
            release(summon, TRUE);
        }
        any |= summon->definition != NULL;
    }
    if (!any && tickDepth == 0)
    {
        memory_free(summons);
        summons = NULL;
    }
}

u32 hackLib_countSummons(Actor *owner, const HackLibSummonDef *definition)
{
    u32 i;
    u32 count = 0;

    reapSummons();
    if (summons == NULL)
    {
        return 0;
    }
    for (i = 0; i < HACKLIB_SUMMON_LIMIT; i++)
    {
        HackLibSummon *summon = &summons[i];
        if (summon->definition != NULL && summon->owner == owner &&
            (definition == NULL || summon->definition == definition) &&
            (summon->definition->isUsable == NULL || summon->definition->isUsable(summon->actor)))
        {
            count++;
        }
    }
    return count;
}

Actor *hackLib_spawnSummon(const HackLibSummonDef *definition, Actor *owner, Vector2Fp16 *position,
                           Actor *target, u16 lifetime, u16 damage, u8 cap, u8 abilityRow)
{
    u32 i;

    if (definition == NULL || definition->spawn == NULL || definition->despawn == NULL ||
        !hackLib_isLiveActor(owner) || hackLib_actorCurrentHp(owner) <= 0 || position == NULL ||
        lifetime <= 1 || hackLib_countSummons(owner, definition) >= cap ||
        hackLib_actorGeneration(owner) == 0)
    {
        return NULL;
    }
    if (summons == NULL)
    {
        summons = memory_zalloc(sizeof(*summons) * HACKLIB_SUMMON_LIMIT);
        if (summons == NULL)
        {
            return NULL;
        }
    }
    for (i = 0; i < HACKLIB_SUMMON_LIMIT; i++)
    {
        if (summons[i].definition == NULL)
        {
            Actor *actor = definition->spawn(owner, position, target, lifetime, damage);
            if (actor != NULL)
            {
                summons[i] = (HackLibSummon){definition,
                                             actor,
                                             owner,
                                             actor->tickCallback,
                                             lifetime,
                                             damage,
                                             abilityRow,
                                             hackLib_actorGeneration(owner),
                                             hackLib_actorGeneration(actor)};
                actor->parentActor = owner;
                actor->tickCallback = tickSummon;
            }
            return actor;
        }
    }
    return NULL;
}

static void tickSummon(Actor *actor)
{
    u32 i;

    if (summons == NULL)
    {
        return;
    }

    for (i = 0; i < HACKLIB_SUMMON_LIMIT; i++)
    {
        HackLibSummon *summon = &summons[i];
        if (summon->definition != NULL && summon->actor == actor)
        {
            const HackLibSummonDef *definition = summon->definition;
            ActorCallback original = summon->originalTick;
            HackLibDamageSource source = {actor, HACKLIB_DAMAGE_SUMMON, summon->abilityRow,
                                          0,     HACKLIB_ELEMENT_NONE,  FALSE};
            const HackLibDamageSource *previous;
            Actor *target = NULL;

            if (!hackLib_isActorRefValid(
                    (HackLibActorRef){summon->owner, summon->ownerGeneration}) ||
                hackLib_actorCurrentHp(summon->owner) <= 0 ||
                (definition->isUsable != NULL && !definition->isUsable(actor)) ||
                (definition->managedLifetime && --summon->remaining <= 1))
            {
                release(summon, TRUE);
                if (original != NULL && !(actor->flags.p & ACTOR_FLAG_PENDING_REMOVE))
                {
                    original(actor);
                }
                reapSummons();
                return;
            }
            if (definition->filter != NULL)
            {
                HackLibTargetQuery query = {.center = actor,
                                            .filter = definition->filter,
                                            .radius = definition->targetRadius};
                target = hackLib_nearestTarget(&query);
            }
            previous = hackLib_swapDamageSource(&source);
            tickDepth++;
            HackLibProfileSpan profile = hackLib_profileBegin(HACKLIB_PROFILE_SUMMONS);
            if (definition->tick != NULL)
            {
                definition->tick(actor, target, summon->damage);
            }
            else if (original != NULL)
            {
                original(actor);
            }
            hackLib_swapDamageSource(previous);
            tickDepth--;
            hackLib_profileEnd(profile);
            reapSummons();
            return;
        }
    }
}

void hackLib_clearOwnerSummons(Actor *owner)
{
    u32 i;
    bool any = FALSE;

    if (summons == NULL)
    {
        return;
    }

    for (i = 0; i < HACKLIB_SUMMON_LIMIT; i++)
    {
        if (summons[i].definition != NULL && summons[i].owner == owner)
        {
            release(&summons[i], TRUE);
        }
        any |= summons[i].definition != NULL;
    }
    if (!any && tickDepth == 0)
    {
        memory_free(summons);
        summons = NULL;
    }
}

u32 hackLib_summonPoolUsed(void)
{
    u32 i;
    u32 count = 0;

    reapSummons();
    if (summons == NULL)
    {
        return 0;
    }
    for (i = 0; i < HACKLIB_SUMMON_LIMIT; i++)
    {
        count += summons[i].definition != NULL;
    }
    return count;
}
