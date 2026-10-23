#include "patches/skill_expansion/skill_delivery.h"
#include "game.h"
#include "lib/movement.h"
#include "lib/skill_fx.h"
#include "lib/status.h"
#include "lib/targeting.h"
#include "memory.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_status.h"
#include "scene.h"
#include "sfx.h"
#include "terrain.h"
#include "variables.h"

#include "skill_delivery_data.inc"

typedef struct DeliveryTarget
{
    HackLibActorRef reference;
    u16 lastTick;
    u8 attempts;
    bool direct;
} DeliveryTarget;

typedef struct Delivery
{
    struct Delivery *next;
    SkillDeliveryDef definition;
    HackLibEffectHandle visual;
    DeliveryTarget targets[SKILL_DELIVERY_TARGET_LIMIT];
    Vector2Fp16 position;
    Vector2Fp16 velocity;
    u16 elapsed;
    u16 damage;
    u16 statusPower;
    u8 count;
    u8 directContacts;
    u8 abilityRow;
    bool cosmetic;
    bool cancelled;
} Delivery;

typedef struct DeliveryContext
{
    Delivery *head;
    HackLibActorRef owner;
    SkillDeliveryStats stats;
    u16 cooldowns[SKILL_EXPANSION_ACTIVE_ROW_COUNT + 1];
    bool stepping;
    bool cancelled;
} DeliveryContext;

static DeliveryContext *contexts[2];

static bool valid(const SkillDeliveryDef *d)
{
    if (d == NULL || (d->elements & ~HACKLIB_ELEMENT_MASK) || d->status > SKILL_STATUS_SLOW ||
        d->statusTicks > 1800 || d->cooldownTicks > 1800 ||
        (d->status != SKILL_STATUS_NONE && d->statusTicks == 0))
    {
        return FALSE;
    }
    for (u32 i = 0; i < 5; i++)
    {
        if (d->statusPower[i] > 21845 ||
            ((d->status == SKILL_STATUS_POISON || d->status == SKILL_STATUS_BURN) &&
             d->statusPower[i] == 0))
        {
            return FALSE;
        }
    }
    return d != NULL && d->kind <= SKILL_DELIVERY_BURST && d->lifetime > 0 && d->lifetime <= 300 &&
           d->radius > 0 && d->radius <= 128 && d->speed <= 16 &&
           (d->kind != SKILL_DELIVERY_STRAIGHT || d->speed > 0) && d->splashRadius <= 128 &&
           d->maxTargets > 0 && d->maxTargets <= SKILL_DELIVERY_TARGET_LIMIT &&
           d->pierceTargets > 0 && d->pierceTargets <= d->maxTargets && d->maxRepeats > 0 &&
           d->maxRepeats <= 8 && d->repeatTicks > 0 && d->trailTicks > 0 && d->knockback <= 32 &&
           d->stunTicks <= 300 && d->launchSound <= 0x11c && d->impactSound <= 0x11c &&
           (d->travelEffect == SKILL_DELIVERY_NO_EFFECT ||
            d->travelEffect < HACKLIB_VISUAL_EFFECT_COUNT) &&
           (d->trailEffect == SKILL_DELIVERY_NO_EFFECT ||
            d->trailEffect < HACKLIB_VISUAL_EFFECT_COUNT) &&
           (d->impactEffect == SKILL_DELIVERY_NO_EFFECT ||
            d->impactEffect < HACKLIB_VISUAL_EFFECT_COUNT);
}

void skillDelivery_getStats(u8 p, SkillDeliveryStats *out)
{
    *out = p < 2 && contexts[p] != NULL ? contexts[p]->stats : (SkillDeliveryStats){0};
}

void skillDelivery_resetStats(u8 p)
{
    if (p < 2 && contexts[p] != NULL)
    {
        u8 active = contexts[p]->stats.active;
        contexts[p]->stats = (SkillDeliveryStats){.active = active};
    }
}

static void reap(u8 p)
{
    DeliveryContext *context = contexts[p];
    Delivery **link = &context->head;
    while (*link != NULL)
    {
        Delivery *delivery = *link;
        if (delivery->cancelled || context->cancelled)
        {
            *link = delivery->next;
            hackLib_stopEffect(&delivery->visual);
            memory_free(delivery);
            context->stats.active--;
        }
        else
        {
            link = &delivery->next;
        }
    }
    if (context->cancelled)
    {
        contexts[p] = NULL;
        memory_free(context);
    }
}

void skillDelivery_clear(u8 p)
{
    if (p >= 2 || contexts[p] == NULL)
    {
        return;
    }
    contexts[p]->cancelled = TRUE;
    if (!contexts[p]->stepping)
    {
        reap(p);
    }
}

bool skillDelivery_launch(u8 p, const SkillDeliveryDef *definition, const Vector2Fp16 *position,
                          u8 direction, u8 level, u8 abilityRow, bool cosmetic)
{
    if (p >= 2 ||
        (abilityRow != HACKLIB_ABILITY_NONE && abilityRow >= SKILL_EXPANSION_ACTIVE_ROW_COUNT) ||
        !valid(definition) || position == NULL || direction >= DIRECTION32_COUNT || level < 1 ||
        level > 5 || !skillExpansion_isPlayerAvatar(PLAYER(p).ownerActor) ||
        hackLib_actorCurrentHp(PLAYER(p).ownerActor) <= 0)
    {
        return FALSE;
    }
    u8 cooldownRow =
        abilityRow == HACKLIB_ABILITY_NONE ? SKILL_EXPANSION_ACTIVE_ROW_COUNT : abilityRow;
    Vector2Int worldSize;
    terrain_getWorldSize(&worldSize);
    if (position->x < 0 || position->y < 0 || (position->x >> 16) >= worldSize.x ||
        (position->y >> 16) >= worldSize.y ||
        !hackLib_isActorRefValid(hackLib_actorRef(PLAYER(p).ownerActor)))
    {
        return FALSE;
    }
    DeliveryContext *context = contexts[p];
    if (context != NULL &&
        (!hackLib_isActorRefValid(context->owner) || context->owner.actor != PLAYER(p).ownerActor))
    {
        skillDelivery_clear(p);
        context = contexts[p];
    }
    if (context != NULL && (context->cancelled || context->stats.active >= SKILL_DELIVERY_LIMIT))
    {
        return FALSE;
    }
    if (!cosmetic && context != NULL && context->cooldowns[cooldownRow] != 0)
    {
        return FALSE;
    }
    Delivery *delivery = memory_zalloc(sizeof(*delivery));
    if (delivery == NULL)
    {
        return FALSE;
    }
    if (context == NULL)
    {
        context = memory_zalloc(sizeof(*context));
        if (context == NULL)
        {
            memory_free(delivery);
            return FALSE;
        }
        context->owner = hackLib_actorRef(PLAYER(p).ownerActor);
        contexts[p] = context;
    }
    delivery->definition = *definition;
    delivery->position = *position;
    delivery->velocity.x = Direction32MoveVectors[direction].x * definition->speed;
    delivery->velocity.y = Direction32MoveVectors[direction].y * definition->speed;
    delivery->damage = definition->damage[level - 1];
    delivery->statusPower = definition->statusPower[level - 1];
    delivery->abilityRow = abilityRow;
    delivery->cosmetic = cosmetic;
    delivery->next = context->head;
    context->head = delivery;
    context->stats.active++;
    if (!cosmetic)
    {
        context->cooldowns[cooldownRow] = definition->cooldownTicks;
    }
    if (definition->travelEffect != SKILL_DELIVERY_NO_EFFECT)
    {
        hackLib_playEffect(definition->travelEffect, context->owner.actor, position,
                           &delivery->visual);
    }
    if (definition->launchSound)
    {
        sfx_play(definition->launchSound);
    }
    return TRUE;
}

static void visual(u16 effect, Actor *owner, const Vector2Fp16 *position)
{
    if (effect != SKILL_DELIVERY_NO_EFFECT)
    {
        hackLib_playEffect(effect, owner, position, NULL);
    }
}

static bool contact(DeliveryContext *context, Delivery *delivery, Actor *target)
{
    const SkillDeliveryDef *d = &delivery->definition;
    if (context->cancelled || delivery->cancelled || !hackLib_isActorRefValid(context->owner) ||
        !skillExpansion_isFoe(target))
    {
        return FALSE;
    }
    DeliveryTarget *entry = NULL;
    HackLibActorRef reference = hackLib_actorRef(target);
    for (u32 i = 0; i < delivery->count; i++)
    {
        if (delivery->targets[i].reference.actor == target &&
            delivery->targets[i].reference.generation == reference.generation)
        {
            entry = &delivery->targets[i];
        }
    }
    if (entry == NULL)
    {
        if (delivery->count >= d->maxTargets)
        {
            return FALSE;
        }
        entry = &delivery->targets[delivery->count++];
        entry->reference = reference;
    }
    if (entry->attempts >= d->maxRepeats ||
        (entry->attempts && delivery->elapsed - entry->lastTick < d->repeatTicks))
    {
        return FALSE;
    }
    entry->attempts++;
    entry->lastTick = delivery->elapsed;
    context->stats.contacts++;
    Vector2Fp16 hitPosition = target->coords;
    if (!delivery->cosmetic)
    {
        HackLibDamageResult result =
            d->elementsKnown
                ? skillExpansion_dealElementalDamage(context->owner.actor, target, delivery->damage,
                                                     HACKLIB_DAMAGE_ABILITY, delivery->abilityRow,
                                                     d->elements)
                : skillExpansion_dealDamage(context->owner.actor, target, delivery->damage,
                                            HACKLIB_DAMAGE_ABILITY, delivery->abilityRow);
        if (result.hit)
        {
            context->stats.hits++;
            context->stats.damage += result.damageDealt > 0 ? result.damageDealt : 0;
            if (!context->cancelled && hackLib_isActorRefValid(context->owner) &&
                hackLib_isActorRefValid(reference) && !result.killed)
            {
                if (d->status != SKILL_STATUS_NONE)
                {
                    skillStatus_apply(d->status, context->owner.actor, target, d->statusTicks,
                                      delivery->statusPower, delivery->abilityRow);
                }
                hackLib_applyStun(target, d->stunTicks);
                s32 dx = (target->coords.x >> 16) - (delivery->position.x >> 16);
                s32 dy = (target->coords.y >> 16) - (delivery->position.y >> 16);
                s32 length = (ABS(dx) > ABS(dy) ? ABS(dx) : ABS(dy));
                if (d->knockback && length)
                {
                    Vector2Fp16 landing = target->coords;
                    // Check each pixel so displacement cannot cross a wall.
                    for (u32 step = 1; step <= d->knockback; step++)
                    {
                        Vector2Fp16 next = {
                            target->coords.x + (s64)dx * step * FP16_16_ONE / length,
                            target->coords.y + (s64)dy * step * FP16_16_ONE / length};
                        if (!hackLib_canLandAt(target, &next))
                        {
                            break;
                        }
                        landing = next;
                    }
                    target->coords = landing;
                }
            }
        }
    }
    if (!context->cancelled && hackLib_isActorRefValid(context->owner))
    {
        visual(d->impactEffect, context->owner.actor, &hitPosition);
        if (d->impactSound)
        {
            sfx_play(d->impactSound);
        }
    }
    return TRUE;
}

static void burst(DeliveryContext *context, Delivery *delivery, const Vector2Fp16 *position,
                  u8 radius)
{
    Actor *target = NULL;
    while (!context->cancelled &&
           (target = hackLib_nextTargetAt(position, context->owner.actor, radius, target)) != NULL)
    {
        contact(context, delivery, target);
    }
}

static bool floorAt(const Vector2Fp16 *position)
{
    Vector2Int size;
    terrain_getWorldSize(&size);
    s32 x = position->x >> 16, y = position->y >> 16;
    return x >= 0 && y >= 0 && x < size.x && y < size.y && (terrain_getCollisionAt(x, y) & 7) == 0;
}

static void straight(DeliveryContext *context, Delivery *delivery)
{
    typedef struct Candidate
    {
        HackLibActorRef target;
        u8 step;
    } Candidate;
    Candidate candidates[SKILL_DELIVERY_TARGET_LIMIT];
    u32 count = 0;
    const SkillDeliveryDef *d = &delivery->definition;
    Vector2Fp16 start = delivery->position;
    Vector2Fp16 unit = {delivery->velocity.x / d->speed, delivery->velocity.y / d->speed};
    s64 endX = (s64)start.x + delivery->velocity.x;
    s64 endY = (s64)start.y + delivery->velocity.y;
    if (endX > 2147483647LL || endX < -2147483648LL || endY > 2147483647LL || endY < -2147483648LL)
    {
        delivery->cancelled = TRUE;
        return;
    }
    Actor *target = NULL;
    // One broad query, then pixel substeps for ordered contacts without tunneling.
    while ((target = hackLib_nextTargetAt(&start, context->owner.actor, d->radius + d->speed + 2,
                                          target)) != NULL)
    {
        if (!skillExpansion_isFoe(target))
        {
            continue;
        }
        for (u32 step = 1; step <= d->speed; step++)
        {
            s32 dx = (target->coords.x >> 16) - ((start.x + unit.x * (s32)step) >> 16);
            s32 dy = (target->coords.y >> 16) - ((start.y + unit.y * (s32)step) >> 16);
            if (dx * dx + dy * dy > d->radius * d->radius)
            {
                continue;
            }
            u32 at = 0;
            while (at < count && candidates[at].step <= step)
            {
                at++;
            }
            if (at < SKILL_DELIVERY_TARGET_LIMIT)
            {
                if (count < SKILL_DELIVERY_TARGET_LIMIT)
                {
                    count++;
                }
                for (u32 i = count - 1; i > at; i--)
                {
                    candidates[i] = candidates[i - 1];
                }
                candidates[at] = (Candidate){hackLib_actorRef(target), step};
            }
            break;
        }
    }
    u32 nextCandidate = 0;
    for (u32 step = 1; step <= d->speed && !context->cancelled && !delivery->cancelled; step++)
    {
        Vector2Fp16 next = {start.x + unit.x * (s32)step, start.y + unit.y * (s32)step};
        if (!floorAt(&next))
        {
            Vector2Int size;
            terrain_getWorldSize(&size);
            if (d->stopOnWalls || next.x < 0 || next.y < 0 || (next.x >> 16) >= size.x ||
                (next.y >> 16) >= size.y)
            {
                visual(d->impactEffect, context->owner.actor, &delivery->position);
                delivery->cancelled = TRUE;
                break;
            }
        }
        delivery->position = next;
        while (nextCandidate < count && candidates[nextCandidate].step == step &&
               !context->cancelled)
        {
            HackLibActorRef ref = candidates[nextCandidate++].target;
            if (!hackLib_isActorRefValid(ref))
            {
                continue;
            }
            Vector2Fp16 hitPosition = ref.actor->coords;
            if (contact(context, delivery, ref.actor))
            {
                for (u32 i = 0; i < delivery->count; i++)
                {
                    DeliveryTarget *entry = &delivery->targets[i];
                    if (entry->reference.actor == ref.actor &&
                        entry->reference.generation == ref.generation && !entry->direct)
                    {
                        entry->direct = TRUE;
                        delivery->directContacts++;
                        break;
                    }
                }
                if (!context->cancelled && d->splashRadius)
                {
                    burst(context, delivery, &hitPosition, d->splashRadius);
                }
                if (delivery->directContacts >= d->pierceTargets)
                {
                    delivery->cancelled = TRUE;
                    break;
                }
            }
        }
    }
}

void skillDelivery_tick(u8 p, Actor *owner)
{
    if (p >= 2 || contexts[p] == NULL)
    {
        return;
    }
    DeliveryContext *context = contexts[p];
    if (context->stepping)
    {
        return;
    }
    if (context->owner.actor != owner || !hackLib_isActorRefValid(context->owner) ||
        hackLib_actorCurrentHp(owner) <= 0)
    {
        skillDelivery_clear(p);
        return;
    }
    if (g_GameFlags.d.overlay)
    {
        return;
    }
    if (g_SceneCurrent.id != SCENE_ID_GAMEPLAY)
    {
        skillDelivery_clear(p);
        return;
    }
    context->stepping = TRUE;
    for (u32 row = 0; row <= SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        if (context->cooldowns[row] != 0)
        {
            context->cooldowns[row]--;
        }
    }
    for (Delivery *delivery = context->head; delivery != NULL && !context->cancelled;
         delivery = delivery->next)
    {
        if (delivery->cancelled)
        {
            continue;
        }
        const SkillDeliveryDef *d = &delivery->definition;
        if (d->kind == SKILL_DELIVERY_STRAIGHT)
        {
            straight(context, delivery);
        }
        else if (delivery->elapsed % d->repeatTicks == 0)
        {
            burst(context, delivery, &delivery->position, d->radius);
        }
        if (context->cancelled || delivery->cancelled)
        {
            continue;
        }
        bool liveVisual = FALSE;
        for (u32 i = 0; i < HACKLIB_EFFECT_LAYER_LIMIT; i++)
        {
            HackLibActorRef ref = delivery->visual.actors[i];
            if (hackLib_isActorRefValid(ref))
            {
                hackLib_fxDetach(ref.actor, &delivery->position);
                liveVisual = TRUE;
            }
        }
        if (!liveVisual && d->travelEffect != SKILL_DELIVERY_NO_EFFECT)
        {
            hackLib_playEffect(d->travelEffect, owner, &delivery->position, &delivery->visual);
        }
        if (d->trailEffect != SKILL_DELIVERY_NO_EFFECT && delivery->elapsed % d->trailTicks == 0)
        {
            visual(d->trailEffect, owner, &delivery->position);
        }
        if (++delivery->elapsed >= d->lifetime)
        {
            delivery->cancelled = TRUE;
        }
    }
    context->stepping = FALSE;
    reap(p);
}
