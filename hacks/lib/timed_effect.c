#include "lib/timed_effect.h"
#include "lib/actor_ref.h"
#include "lib/combat_source.h"
#include "lib/profile.h"
#include "lib/targeting.h"
#include "memory.h"

static HackLibTimedEffect *effects;
static u8 tickDepth;

static void tickEffects(Actor *actor);

static bool isAttached(const HackLibTimedEffect *effect)
{
    return hackLib_isActorRefValid((HackLibActorRef){effect->target, effect->targetGeneration}) &&
           effect->target->tickCallback == tickEffects;
}

void hackLib_removeEffect(HackLibTimedEffect *effect)
{
    Actor *target;
    ActorCallback original;
    u32 i;
    bool attached;

    if (effect == NULL || effect->definition == NULL)
    {
        return;
    }
    target = effect->target;
    original = effect->originalTick;
    attached = isAttached(effect);
    if (attached && effect->definition->remove != NULL)
    {
        effect->definition->remove(effect);
    }
    memory_memset(effect, 0, sizeof(*effect));
    for (i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
    {
        if (effects[i].definition != NULL && effects[i].target == target)
        {
            return;
        }
    }
    if (attached)
    {
        target->tickCallback = original;
    }
}

static void reapEffects(void)
{
    u32 i;
    bool any = FALSE;

    if (effects == NULL)
    {
        return;
    }

    for (i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
    {
        if (effects[i].definition != NULL &&
            (!isAttached(&effects[i]) || !hackLib_isActorRefValid((HackLibActorRef){
                                             effects[i].owner, effects[i].ownerGeneration})))
        {
            hackLib_removeEffect(&effects[i]);
        }
        any |= effects[i].definition != NULL;
    }
    if (!any && tickDepth == 0)
    {
        memory_free(effects);
        effects = NULL;
    }
}

HackLibTimedEffect *hackLib_applyEffect(const HackLibEffectDef *definition, Actor *owner,
                                        Actor *target, u16 frames, s16 magnitude, u8 abilityRow)
{
    HackLibTimedEffect *slot = NULL;
    ActorCallback original;
    u32 i;

    if (definition == NULL || definition->id == 0 || definition->maxStacks == 0 ||
        definition->stackRule > HACKLIB_EFFECT_REPLACE || definition->movementPercent > 100 ||
        frames == 0 || !hackLib_isLiveActor(owner) || !hackLib_isLiveActor(target) ||
        hackLib_actorCurrentHp(target) <= 0)
    {
        return NULL;
    }
    if (hackLib_actorGeneration(owner) == 0 || hackLib_actorGeneration(target) == 0)
    {
        return NULL;
    }
    reapEffects();
    if (effects == NULL)
    {
        effects = memory_zalloc(sizeof(*effects) * HACKLIB_EFFECT_LIMIT);
        if (effects == NULL)
        {
            return NULL;
        }
    }
    original = target->tickCallback;
    for (i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
    {
        HackLibTimedEffect *effect = &effects[i];

        if (effect->definition == NULL)
        {
            slot = effect;
            continue;
        }
        if (effect->target == target)
        {
            original = effect->originalTick;
        }
        if (effect->target == target && effect->owner == owner &&
            effect->definition->id == definition->id)
        {
            if (effect->definition != definition)
            {
                return NULL;
            }
            if (definition->stackRule == HACKLIB_EFFECT_REPLACE)
            {
                hackLib_removeEffect(effect);
                slot = effect;
                break;
            }
            if (frames > effect->remaining)
            {
                effect->remaining = frames;
            }
            if (definition->stackRule == HACKLIB_EFFECT_STACK &&
                effect->stacks < definition->maxStacks)
            {
                effect->stacks++;
            }
            return effect;
        }
    }
    if (slot == NULL)
    {
        return NULL;
    }
    *slot = (HackLibTimedEffect){definition,
                                 owner,
                                 target,
                                 original,
                                 frames,
                                 definition->interval,
                                 magnitude,
                                 1,
                                 abilityRow,
                                 hackLib_actorGeneration(owner),
                                 hackLib_actorGeneration(target)};
    target->tickCallback = tickEffects;
    if (definition->apply != NULL)
    {
        definition->apply(slot);
    }
    return slot;
}

static void tickEffects(Actor *actor)
{
    ActorCallback original = NULL;
    HackLibActorRef reference = hackLib_actorRef(actor);
    Vector2Fp16 before = actor->coords;
    u8 movementPercent = 100;
    u32 i;

    if (effects == NULL)
    {
        return;
    }
    tickDepth++;
    HackLibProfileSpan profile = hackLib_profileBegin(HACKLIB_PROFILE_EFFECTS);

    for (i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
    {
        HackLibTimedEffect *effect = &effects[i];

        if (effect->definition == NULL || effect->target != actor)
        {
            continue;
        }
        original = effect->originalTick;
        if (hackLib_actorCurrentHp(actor) <= 0 ||
            !hackLib_isActorRefValid((HackLibActorRef){effect->owner, effect->ownerGeneration}))
        {
            hackLib_removeEffect(effect);
            continue;
        }
        if (effect->definition->movementPercent != 0 &&
            effect->definition->movementPercent < movementPercent)
        {
            movementPercent = effect->definition->movementPercent;
        }
        if (effect->untilPulse != 0 && --effect->untilPulse == 0)
        {
            effect->untilPulse = effect->definition->interval;
            if (effect->definition->pulse != NULL)
            {
                HackLibDamageSource source = {effect->owner,        HACKLIB_DAMAGE_PERIODIC,
                                              effect->abilityRow,   0,
                                              HACKLIB_ELEMENT_NONE, FALSE};
                const HackLibDamageSource *previous = hackLib_swapDamageSource(&source);
                effect->definition->pulse(effect);
                hackLib_swapDamageSource(previous);
            }
        }
        if (effect->definition != NULL && --effect->remaining == 0)
        {
            hackLib_removeEffect(effect);
        }
    }
    tickDepth--;
    hackLib_profileEnd(profile);
    reapEffects();
    if (original != NULL && !(actor->flags.p & ACTOR_FLAG_PENDING_REMOVE))
    {
        original(actor);
        if (movementPercent < 100 && hackLib_isActorRefValid(reference) &&
            hackLib_actorCurrentHp(actor) > 0 && !(actor->flags.p & ACTOR_FLAG_PENDING_REMOVE) &&
            (s64)actor->coords.x - before.x <= 16 * FP16_16_ONE &&
            (s64)actor->coords.x - before.x >= -16 * FP16_16_ONE &&
            (s64)actor->coords.y - before.y <= 16 * FP16_16_ONE &&
            (s64)actor->coords.y - before.y >= -16 * FP16_16_ONE)
        {
            actor->coords.x = before.x + ((s64)actor->coords.x - before.x) * movementPercent / 100;
            actor->coords.y = before.y + ((s64)actor->coords.y - before.y) * movementPercent / 100;
        }
    }
}

void hackLib_clearOwnerEffects(Actor *owner)
{
    u32 i;
    bool any = FALSE;

    if (effects == NULL)
    {
        return;
    }

    for (i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
    {
        if (effects[i].definition != NULL && effects[i].owner == owner)
        {
            hackLib_removeEffect(&effects[i]);
        }
        any |= effects[i].definition != NULL;
    }
    if (!any && tickDepth == 0)
    {
        memory_free(effects);
        effects = NULL;
    }
}

u32 hackLib_countEffects(Actor *owner)
{
    u32 i;
    u32 count = 0;

    reapEffects();
    if (effects == NULL)
    {
        return 0;
    }
    for (i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
    {
        count += effects[i].definition != NULL && effects[i].owner == owner;
    }
    return count;
}

u32 hackLib_effectPoolUsed(void)
{
    u32 i;
    u32 count = 0;

    reapEffects();
    if (effects == NULL)
    {
        return 0;
    }
    for (i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
    {
        count += effects[i].definition != NULL;
    }
    return count;
}

const HackLibTimedEffect *hackLib_effectAt(u32 index)
{
    reapEffects();
    return effects != NULL && index < HACKLIB_EFFECT_LIMIT && effects[index].definition != NULL
               ? &effects[index]
               : NULL;
}

bool hackLib_actorHasTick(Actor *actor, ActorCallback callback)
{
    u32 i;

    if (actor->tickCallback == callback)
    {
        return TRUE;
    }
    if (effects != NULL && actor->tickCallback == tickEffects)
    {
        for (i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
        {
            if (effects[i].definition != NULL && effects[i].target == actor &&
                effects[i].originalTick == callback)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void hackLib_replaceActorTick(Actor *actor, ActorCallback before, ActorCallback after)
{
    u32 i;

    if (actor->tickCallback == before)
    {
        actor->tickCallback = after;
    }
    else if (effects != NULL && actor->tickCallback == tickEffects)
    {
        for (i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
        {
            if (effects[i].definition != NULL && effects[i].target == actor &&
                effects[i].originalTick == before)
            {
                effects[i].originalTick = after;
            }
        }
    }
}
