#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "actor/projectile.h"
#include "lib/actor_ref.h"
#include "lib/status.h"
#include "lib/summon_registry.h"
#include "lib/targeting.h"

#define ABILITY_PROJECTILE_LIMIT 8

typedef struct AbilityProjectile
{
    Actor *actor;
    u16 damage;
    u8 abilityRow;
    u32 generation;
} AbilityProjectile;

static AbilityProjectile projectiles[ABILITY_PROJECTILE_LIMIT];

Actor *skillExpansion_findTarget(Actor *origin, u16 range, SkillExpansionTargetCondition condition,
                                 u32 argument)
{
    HackLibTargetQuery query = {.center = origin, .radius = range > 32767 ? 32767 : range};
    Actor *target = NULL;
    Actor *best = NULL;
    u32 bestDistance = 0;

    while ((target = hackLib_nextTarget(&query, target)) != NULL)
    {
        s32 dx = (target->coords.x - origin->coords.x) >> FP16_16_SHIFT;
        s32 dy = (target->coords.y - origin->coords.y) >> FP16_16_SHIFT;
        u32 distance = (u32)(dx * dx) + (u32)(dy * dy);
        if (skillExpansion_isFoe(target) && (condition == NULL || condition(target, argument)) &&
            (best == NULL || distance < bestDistance))
        {
            best = target;
            bestDistance = distance;
        }
    }
    return best;
}

u32 skillExpansion_burstDamage(const SkillExpansionBurst *burst)
{
    Actor *target = NULL;
    u32 hits = 0;

    if (burst == NULL || burst->attacker == NULL || burst->center == NULL)
    {
        return 0;
    }

    Vector2Fp16 center = burst->center->coords;
    while ((target = hackLib_nextTargetAt(&center, burst->exclude,
                                          burst->radius > 32767 ? 32767 : burst->radius, target)) !=
           NULL)
    {
        if (target != burst->exclude && skillExpansion_isFoe(target))
        {
            HackLibDamageResult result = skillExpansion_dealDamage(
                burst->attacker, target, burst->damage, burst->kind, burst->abilityRow);
            if (result.hit)
            {
                hits++;
                if (!result.killed)
                {
                    hackLib_applyStun(target, burst->stunFrames);
                }
            }
        }
    }
    return hits;
}

static bool isActiveProjectile(const AbilityProjectile *entry);

static void projectileHit(Actor *projectile, Actor *target)
{
    u32 i;

    if ((projectile->flags.p & ACTOR_FLAG_PENDING_REMOVE) || !skillExpansion_isFoe(target) ||
        !skillExpansion_isPlayerAvatar(projectile->parentActor))
    {
        return;
    }
    for (i = 0; i < ABILITY_PROJECTILE_LIMIT; i++)
    {
        if (projectiles[i].actor == projectile && isActiveProjectile(&projectiles[i]))
        {
            HackLibDamageResult result =
                skillExpansion_dealDamage(projectile->parentActor, target, projectiles[i].damage,
                                          HACKLIB_DAMAGE_ABILITY, projectiles[i].abilityRow);
            if (result.hit)
            {
                projectile->flags.p |= ACTOR_FLAG_PENDING_REMOVE;
            }
            return;
        }
    }
}

static bool isActiveProjectile(const AbilityProjectile *entry)
{
    return hackLib_isActorRefValid((HackLibActorRef){entry->actor, entry->generation}) &&
           entry->actor->triggerCallbacks[0] == projectileHit;
}

Actor *skillExpansion_launchProjectile(Actor *owner, u8 direction32, u16 damage, u8 abilityRow)
{
    u32 i;

    if (!skillExpansion_isPlayerAvatar(owner) || direction32 >= DIRECTION32_COUNT)
    {
        return NULL;
    }
    for (i = 0; i < ABILITY_PROJECTILE_LIMIT; i++)
    {
        if (!isActiveProjectile(&projectiles[i]))
        {
            Actor *projectile = actor_projectile_create(owner, direction32, TRUE);

            if (projectile != NULL)
            {
                projectiles[i] = (AbilityProjectile){projectile, damage, abilityRow,
                                                     hackLib_actorGeneration(projectile)};
                projectile->triggerCallbacks[0] = projectileHit;
            }
            return projectile;
        }
    }
    return NULL;
}

static Actor *spawnGhost(Actor *owner, Vector2Fp16 *position, Actor *target, u16 lifetime,
                         u16 damage)
{
    Actor *ghost = spawnNpc(GHOST_NPC_KIND, position, owner, target, lifetime);
    if (ghost != NULL)
    {
        ghostState(ghost)->damage = damage > 255 ? 255 : damage;
        owner->as.combat.comboState[0]++;
        skillExpansion_trackGhost(ghost);
    }
    return ghost;
}

static void despawnGhost(Actor *ghost)
{
    ghostState(ghost)->lifetime = 1;
}

static bool32 isUsableGhost(Actor *ghost)
{
    return ghostState(ghost)->lifetime > 1 && ghost->behaviorState != GHOST_BEHAVIOR_DESPAWNING;
}

static void tickPersistentGhost(Actor *ghost, Actor *target, u16 damage)
{
    (void)target;
    (void)damage;
    hackLib_tickPersistentGhost(ghost, skillExpansion_isFoe, findNextEnemy);
}

static const HackLibSummonDef GhostSummon = {
    .spawn = spawnGhost,
    .despawn = despawnGhost,
    .isUsable = isUsableGhost,
};
static const HackLibSummonDef PersistentGhostSummon = {
    .spawn = spawnGhost,
    .tick = tickPersistentGhost,
    .despawn = despawnGhost,
    .isUsable = isUsableGhost,
};

Actor *skillExpansion_summonGhost(Actor *owner, Vector2Fp16 *position, Actor *target, u16 lifetime,
                                  u16 damage, u8 cap, bool persistent)
{
    const HackLibDamageSource *source = hackLib_currentDamageSource();
    if (!skillExpansion_isPlayerAvatar(owner) || owner->as.combat.comboState[0] >= cap)
    {
        return NULL;
    }
    return hackLib_spawnSummon(persistent ? &PersistentGhostSummon : &GhostSummon, owner, position,
                               target, lifetime, damage, cap,
                               source != NULL ? source->abilityRow : HACKLIB_ABILITY_NONE);
}

u32 skillExpansion_projectilePoolUsed(void)
{
    u32 count = 0;
    for (u32 i = 0; i < ABILITY_PROJECTILE_LIMIT; i++)
    {
        count += isActiveProjectile(&projectiles[i]);
    }
    return count;
}

bool skillExpansion_consumeGhost(Actor *ghost, Actor *owner)
{
    if (ghost == NULL || ghost->type != ACTOR_TYPE_NPC || ghost->variant != GHOST_NPC_KIND ||
        ghost->parentActor != owner || ghostState(ghost)->lifetime <= 1 ||
        ghost->behaviorState == GHOST_BEHAVIOR_DESPAWNING)
    {
        return FALSE;
    }
    ghostState(ghost)->lifetime = 1;
    return TRUE;
}
