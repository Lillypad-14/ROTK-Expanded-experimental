#include "patches/gauntlet/gauntlet_boon_effects.h"

#include "actor/lootPile.h"
#include "enemy.h"
#include "game.h"
#include "lib/actor_ref.h"
#include "lib/fx_kit.h"
#include "lib/movement.h"
#include "lib/status.h"
#include "lib/summon.h"
#include "lib/targeting.h"
#include "loot.h"
#include "memory.h"
#include "patches/gauntlet/gauntlet.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "skill.h"
#include "stats.h"
#include "variables.h"

#define HAS(name) gauntletBoons_has(GAUNTLET_BOON_##name)
#define SECOND 30
#define STATUS_LIMIT 8
#define REPEAT_LIMIT 4

typedef struct BoonStatus
{
    HackLibActorRef reference;
    s16 x;
    s16 y;
    u8 burn;
    u8 venom;
    u8 frost;
    u8 bleed;
    u8 dread;
    u8 venomStacks;
    u8 frostStacks;
    u8 frozen;
} BoonStatus;

typedef struct BoonRepeat
{
    HackLibActorRef target;
    u16 damage;
    u8 ticks;
    u8 kind;
} BoonRepeat;

typedef struct GauntletBoonEffects
{
    BoonStatus status[STATUS_LIMIT];
    BoonRepeat repeats[REPEAT_LIMIT];
    s16 statBase[4];
    s16 statApplied[4];
    Vector2Fp16 fissure;
    u16 idle;
    u16 paidSpirit;
    u8 tick;
    u8 cooldownFraction;
    u8 attackFraction;
    u8 rapid;
    u8 rapidTicks;
    u8 immunity;
    u8 mirror;
    u8 reflect;
    u8 previousRitualRow;
    u8 previousRitual;
    u8 previousWeave;
    u8 previousPower;
    u8 ritualRow;
    u8 ritual;
    u8 weave;
    u8 rowPower[SKILL_EXPANSION_ACTIVE_ROW_COUNT];
    u8 echoRow;
    u8 echoTicks;
    u8 fissureTicks;
    u8 thornHits;
    u8 summonKills;
    bool handlingEvent;
    bool echoing;
    bool statsReady;
} GauntletBoonEffects;

static GauntletBoonEffects *effects;
static u16 ringLoss;
#define Extra (*effects)

static bool ensureEffects(void)
{
    if (game_isSceneChangePending())
    {
        return FALSE;
    }
    if (effects == NULL && gauntletBoons_hasExpanded())
    {
        effects = memory_zalloc(sizeof(*effects));
    }
    return effects != NULL;
}

static bool usable(BoonStatus *status)
{
    return hackLib_isActorRefValid(status->reference) &&
           skillExpansion_isFoe(status->reference.actor);
}

static BoonStatus *findStatus(Actor *target, bool create)
{
    if (!ensureEffects())
    {
        return NULL;
    }
    BoonStatus *freeSlot = NULL;
    for (u32 i = 0; i < STATUS_LIMIT; i++)
    {
        BoonStatus *status = &Extra.status[i];
        if (status->reference.actor == target && hackLib_isActorRefValid(status->reference))
        {
            return status;
        }
        if (!usable(status))
        {
            *status = (BoonStatus){0};
            freeSlot = status;
        }
        else if (status->reference.actor == target)
        {
            return status;
        }
    }
    if (!create || freeSlot == NULL || !skillExpansion_isFoe(target))
    {
        return NULL;
    }
    freeSlot->reference = hackLib_actorRef(target);
    freeSlot->x = target->coords.x >> 16;
    freeSlot->y = target->coords.y >> 16;
    return freeSlot;
}

static void damage(Actor *owner, Actor *target, s32 amount, u8 kind)
{
    if (amount > 0 && skillExpansion_isFoe(target))
    {
        if ((effects != NULL && Extra.handlingEvent) || hackLib_currentDamageSource() != NULL)
        {
            gauntletBoonEffects_queue(target, amount, kind, 1);
            return;
        }
        skillExpansion_dealDamage(owner, target, amount > 32767 ? 32767 : amount, kind,
                                  HACKLIB_ABILITY_NONE);
    }
}

static Actor *nearest(Actor *owner, const Vector2Fp16 *position, Actor *exclude, u16 radius)
{
    Actor *cursor = NULL;
    Actor *best = NULL;
    s32 bestDistance = 0x7fffffff;
    (void)owner;
    while ((cursor = hackLib_nextTargetAt(position, exclude, radius, cursor)) != NULL)
    {
        s32 dx = (cursor->coords.x - position->x) >> 16;
        s32 dy = (cursor->coords.y - position->y) >> 16;
        s32 distance = dx * dx + dy * dy;
        if (skillExpansion_isFoe(cursor) && distance < bestDistance)
        {
            best = cursor;
            bestDistance = distance;
        }
    }
    return best;
}

static void burst(Actor *owner, const Vector2Fp16 *position, Actor *exclude, s32 amount, u16 radius,
                  u8 shape, HackLibFxColor color)
{
    Actor *cursor = NULL;
    hackLib_fxAt(owner, position, shape, color);
    while ((cursor = hackLib_nextTargetAt(position, exclude, radius, cursor)) != NULL)
    {
        damage(owner, cursor, amount, HACKLIB_DAMAGE_SECONDARY);
    }
}

void gauntletBoons_ignite(Actor *target)
{
    BoonStatus *status = findStatus(target, TRUE);
    if (status != NULL)
    {
        status->burn = 3 * SECOND;
    }
}

static void chill(Actor *target, u8 stacks)
{
    BoonStatus *status = findStatus(target, TRUE);
    if (status != NULL)
    {
        status->frost = 3 * SECOND;
        status->frostStacks += stacks;
        if (status->frostStacks >= 5)
        {
            status->frostStacks = 0;
            status->frozen = 2 * SECOND;
            hackLib_applyStun(target, 2 * SECOND);
            hackLib_fxBurst(target, FX_SHAPE_DIAMOND_RING, FX_COLOR_BLUE, FX_LAYER_FRONT);
        }
    }
}

static void venom(Actor *target, u8 stacks)
{
    BoonStatus *status = findStatus(target, TRUE);
    if (status != NULL)
    {
        status->venom = 3 * SECOND;
        u32 total = status->venomStacks + stacks;
        status->venomStacks = total > 10 ? 10 : total;
    }
}

static void dreadAround(Actor *owner, const Vector2Fp16 *position, u16 radius)
{
    Actor *cursor = NULL;
    while ((cursor = hackLib_nextTargetAt(position, owner, radius, cursor)) != NULL)
    {
        BoonStatus *status = findStatus(cursor, TRUE);
        if (status != NULL)
        {
            status->dread = 3 * SECOND;
        }
    }
}

static void relocate(Actor *actor, Vector2Fp16 position)
{
    if (hackLib_canLandAt(actor, &position))
    {
        actor->renderCoords.x += position.x - actor->coords.x;
        actor->renderCoords.y += position.y - actor->coords.y;
        actor->coords = position;
    }
}

static void statusTick(Actor *owner)
{
    for (u32 i = 0; i < STATUS_LIMIT; i++)
    {
        BoonStatus *status = &Extra.status[i];
        if (!usable(status))
        {
            *status = (BoonStatus){0};
            continue;
        }
        Actor *target = status->reference.actor;
        s32 x = target->coords.x >> 16;
        s32 y = target->coords.y >> 16;
        bool moved = x != status->x || y != status->y;
        if (status->frost != 0 && status->frozen == 0 && moved)
        {
            Vector2Fp16 position = {target->coords.x - (x - status->x) * 16384,
                                    target->coords.y - (y - status->y) * 16384};
            relocate(target, position);
        }
        if (status->dread != 0)
        {
            Vector2Fp16 position = {status->x * 65536, status->y * 65536};
            position.x += target->coords.x < owner->coords.x ? -65536 : 65536;
            position.y += target->coords.y < owner->coords.y ? -65536 : 65536;
            relocate(target, position);
        }
        bool expires = status->burn == 1 || status->venom == 1 || status->bleed == 1;
        if (Extra.tick == 0)
        {
            s32 amount = (status->burn != 0 ? 3 : 0) +
                         (status->venom != 0 ? status->venomStacks : 0) +
                         (status->bleed != 0 && moved ? 4 : 0);
            if (status->burn != 0 && HAS(WILDFIRE_SEED))
            {
                Actor *cursor = NULL;
                while ((cursor = hackLib_nextTargetAt(&target->coords, target, 24, cursor)) != NULL)
                {
                    if (skillExpansion_isFoe(cursor))
                    {
                        gauntletBoons_ignite(cursor);
                    }
                }
            }
            damage(owner, target, amount, HACKLIB_DAMAGE_PERIODIC);
        }
        if (!usable(status))
        {
            continue;
        }
        if (expires && HAS(DETONATE))
        {
            Vector2Fp16 center = target->coords;
            burst(owner, &center, NULL, 12, 40, FX_SHAPE_POOL_FLASH, FX_COLOR_EMBER);
        }
        if (!usable(status))
        {
            continue;
        }
        if (status->burn)
        {
            status->burn--;
        }
        if (status->venom)
        {
            status->venom--;
        }
        else
        {
            status->venomStacks = 0;
        }
        if (status->frost)
        {
            status->frost--;
        }
        else
        {
            status->frostStacks = 0;
        }
        if (status->bleed)
        {
            status->bleed--;
        }
        if (status->dread)
        {
            status->dread--;
        }
        if (status->frozen)
        {
            status->frozen--;
        }
        status->x = target->coords.x >> 16;
        status->y = target->coords.y >> 16;
    }
}

bool gauntletBoonEffects_queue(Actor *target, s32 amount, u8 kind, u8 ticks)
{
    if (!ensureEffects() || amount <= 0 || !skillExpansion_isFoe(target))
    {
        return FALSE;
    }
    BoonRepeat *slot = NULL;
    for (u32 i = 0; i < REPEAT_LIMIT; i++)
    {
        BoonRepeat *repeat = &Extra.repeats[i];
        if (ticks == 1 && repeat->ticks == 1 && repeat->kind == kind &&
            repeat->target.actor == target && hackLib_isActorRefValid(repeat->target))
        {
            amount += repeat->damage;
            slot = repeat;
            break;
        }
        if (repeat->ticks == 0)
        {
            slot = repeat;
        }
    }
    if (slot == NULL)
    {
        return FALSE;
    }
    slot->target = hackLib_actorRef(target);
    slot->damage = amount > 32767 ? 32767 : amount;
    slot->ticks = ticks;
    slot->kind = kind;
    return TRUE;
}

static void queueRepeat(Actor *target, s32 amount)
{
    gauntletBoonEffects_queue(target, amount, HACKLIB_DAMAGE_SECONDARY, SECOND / 2);
}

void gauntletBoonEffects_statsRebuilt(void)
{
    if (effects != NULL)
    {
        Extra.statsReady = FALSE;
    }
}

void gauntletBoonEffects_roomEnd(void)
{
    if (effects == NULL)
    {
        return;
    }
    static const u8 indices[] = {STAT_CRIT_CHANCE_PERCENT, STAT_SPEED_PERCENT,
                                 STAT_EXTRA_GEMS_PERCENT, STAT_MAX_HP};
    if (Extra.statsReady)
    {
        for (u32 i = 0; i < 4; i++)
        {
            // BUG: Gear can change the stat after the last tick without removing its overlay.
            PLAYER_STAT(0, indices[i]) -= Extra.statApplied[i] - Extra.statBase[i];
        }
        PLAYER(0).combatFlags.p |= PLAYER_COMBAT_FLAG_MOVE_SPEED_DIRTY;
    }
    memory_free(effects);
    effects = NULL;
}

void gauntletBoonEffects_reset(void)
{
    gauntletBoonEffects_roomEnd();
    ringLoss = 0;
}

void gauntletBoonEffects_roomStart(void)
{
    gauntletBoonEffects_roomEnd();
    if (HAS(THE_ONE_RING))
    {
        s32 loss = PLAYER_STAT(0, STAT_MAX_HP) / 100;
        if (loss < 1)
        {
            loss = 1;
        }
        u32 total = ringLoss + loss;
        ringLoss = total > 32767 ? 32767 : total;
    }
}

static void statTick(u8 player)
{
    static const u8 indices[] = {STAT_CRIT_CHANCE_PERCENT, STAT_SPEED_PERCENT,
                                 STAT_EXTRA_GEMS_PERCENT, STAT_MAX_HP};
    s16 bonuses[] = {HAS(KEEN_EYE) ? 10 : 0, HAS(FLEETFOOT) ? 12 : 0, HAS(DWARF_GOLD) ? 40 : 0,
                     -(s16)ringLoss};
    for (u32 i = 0; i < 4; i++)
    {
        s16 current = PLAYER_STAT(player, indices[i]);
        if (!Extra.statsReady)
        {
            Extra.statBase[i] = current;
        }
        else if (current != Extra.statApplied[i])
        {
            // BUG: Incremental equipment changes already include the previous boon overlay.
            Extra.statBase[i] += current - Extra.statApplied[i];
        }
        Extra.statApplied[i] = Extra.statBase[i] + bonuses[i];
        if (indices[i] == STAT_MAX_HP && Extra.statApplied[i] < 1)
        {
            Extra.statApplied[i] = 1;
        }
        PLAYER_STAT(player, indices[i]) = Extra.statApplied[i];
        if (indices[i] == STAT_SPEED_PERCENT && current != Extra.statApplied[i])
        {
            PLAYER(player).combatFlags.p |= PLAYER_COMBAT_FLAG_MOVE_SPEED_DIRTY;
        }
    }
    Extra.statsReady = TRUE;
    if (PLAYER(player).currentHp > PLAYER_STAT(player, STAT_MAX_HP))
    {
        PLAYER(player).currentHp = PLAYER_STAT(player, STAT_MAX_HP);
    }
}

static void gemsTick(u8 player, Actor *owner)
{
    Actor *cursor = g_ActorListMain.head;
    while (cursor != NULL)
    {
        Actor *next = cursor->next;
        if (cursor->type == ACTOR_TYPE_LOOT_PILE &&
            (cursor->variant == LOOT_TYPE_GEM_STACK ||
             cursor->variant == LOOT_TYPE_GEM_STACK_MIN) &&
            !(cursor->flags.p & ACTOR_FLAG_PENDING_REMOVE))
        {
            s32 dx = (owner->coords.x - cursor->coords.x) >> 16;
            s32 dy = (owner->coords.y - cursor->coords.y) >> 16;
            if (ABS(dx) <= 240 && ABS(dy) <= 240)
            {
                if (ABS(dx) <= 12 && ABS(dy) <= 12)
                {
                    u16 before = PLAYER(player).gems;
                    actor_lootPile_trigger(cursor, owner);
                    if (PLAYER(player).gems > before)
                    {
                        skillExpansion_addHp(player, PLAYER_STAT(player, STAT_MAX_HP) / 100 + 1,
                                             FALSE);
                    }
                }
                else
                {
                    s32 x = dx < -4 ? -4 : dx > 4 ? 4 : dx;
                    s32 y = dy < -4 ? -4 : dy > 4 ? 4 : dy;
                    cursor->coords.x += x * 65536;
                    cursor->coords.y += y * 65536;
                    cursor->renderCoords = cursor->coords;
                }
            }
        }
        cursor = next;
    }
}

void gauntletBoonEffects_tick(u8 player, Actor *owner)
{
    if (!ensureEffects())
    {
        return;
    }
    Extra.tick = (Extra.tick + 1) % SECOND;
    if (Extra.idle < 65535)
    {
        Extra.idle++;
    }
    if (Extra.immunity)
    {
        Extra.immunity--;
    }
    if (Extra.mirror)
    {
        Extra.mirror--;
    }
    if (Extra.reflect)
    {
        Extra.reflect--;
    }
    if (Extra.rapidTicks && --Extra.rapidTicks == 0)
    {
        Extra.rapid = 0;
    }
    statTick(player);
    if (HAS(RANGERS_PATIENCE))
    {
        Extra.cooldownFraction += 15;
        if (Extra.cooldownFraction >= 100)
        {
            Extra.cooldownFraction -= 100;
            for (u32 row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
            {
                if (skillExpansion_runtime[player].cooldowns[row])
                {
                    skillExpansion_runtime[player].cooldowns[row]--;
                }
            }
        }
    }
    const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(PLAYER(player).heroId);
    u32 artHero = kit != NULL ? kit->artHeroId : PLAYER(player).heroId;
    bool attacking = artHero < ARRAY_COUNT(HeroMeleeAnimationBanks) &&
                     owner->as.combat.head.player.animationBank == HeroMeleeAnimationBanks[artHero];
    if (attacking && (HAS(SWIFT_BLADE) || Extra.rapid))
    {
        Extra.attackFraction += (HAS(SWIFT_BLADE) ? 15 : 0) + Extra.rapid * 5;
        if (Extra.attackFraction >= 100)
        {
            Extra.attackFraction -= 100;
            if (owner->animationFramesLeft > 1)
            {
                owner->animationFramesLeft--;
            }
        }
    }
    for (u32 i = 0; i < REPEAT_LIMIT; i++)
    {
        BoonRepeat *repeat = &Extra.repeats[i];
        if (repeat->ticks && --repeat->ticks == 0 && hackLib_isActorRefValid(repeat->target))
        {
            Actor *target = repeat->target.actor;
            hackLib_fxBurst(target, FX_SHAPE_SPARK, FX_COLOR_GHOST, FX_LAYER_FRONT);
            damage(owner, target, repeat->damage, repeat->kind);
        }
    }
    if (Extra.echoTicks && --Extra.echoTicks == 0 && !skillExpansion_runtime[player].pending)
    {
        Extra.echoing = TRUE;
        Actor *target = nearest(owner, &owner->coords, owner, 128);
        if (target != NULL)
        {
            u8 rank = *skillExpansion_activeLevel(player, Extra.echoRow);
            hackLib_fxBurst(target, FX_SHAPE_SPARK, FX_COLOR_GHOST, FX_LAYER_FRONT);
            damage(owner, target, 10 + rank * 6, HACKLIB_DAMAGE_SECONDARY);
        }
        gauntletBoons_castResolved(player, Extra.echoRow, TRUE);
        Extra.echoing = FALSE;
    }
    if (Extra.reflect && Extra.tick % 3 == 0)
    {
        for (Actor *cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
        {
            Actor *origin = cursor->parentActor;
            if (cursor->type == ACTOR_TYPE_PROJECTILE && origin != owner &&
                !(cursor->flags.p & ACTOR_FLAG_PENDING_REMOVE) &&
                ABS((cursor->coords.x - owner->coords.x) >> 16) <= 48 &&
                ABS((cursor->coords.y - owner->coords.y) >> 16) <= 48)
            {
                cursor->flags.p |= ACTOR_FLAG_PENDING_REMOVE;
                if (hackLib_isActorRefValid(hackLib_actorRef(origin)))
                {
                    damage(owner, origin, 10, HACKLIB_DAMAGE_SECONDARY);
                }
            }
        }
    }
    if (HAS(GEM_MAGNET))
    {
        gemsTick(player, owner);
    }
    if (Extra.tick == 0 && HAS(OVERFLOW))
    {
        s32 cap = PLAYER_STAT(player, STAT_MAX_SPIRIT);
        s32 spirit = PLAYER(player).currentSpirit + cap * 2 / 100 + 1;
        PLAYER(player).currentSpirit = spirit > cap * 3 / 2 ? cap * 3 / 2 : spirit;
    }
    if (Extra.tick == 0 && HAS(WANDERING_STAR))
    {
        Actor *target = nearest(owner, &owner->coords, owner, 128);
        if (target != NULL)
        {
            hackLib_fxBurst(target, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
            damage(owner, target, 10, HACKLIB_DAMAGE_SECONDARY);
        }
    }
    if (Extra.tick % 15 == 0 && HAS(EMBER_TRAIL) && (owner->velocity.x || owner->velocity.y))
    {
        Actor *cursor = NULL;
        while ((cursor = hackLib_nextTargetAt(&owner->coords, owner, 32, cursor)) != NULL)
        {
            if (skillExpansion_isFoe(cursor))
            {
                gauntletBoons_ignite(cursor);
            }
        }
        hackLib_fxAt(owner, &owner->coords, FX_SHAPE_POOL_FLASH, FX_COLOR_EMBER);
    }
    if (HAS(LIGHT_OF_EARENDIL) && Extra.tick == 0)
    {
        dreadAround(owner, &owner->coords, 72);
    }
    if (Extra.fissureTicks)
    {
        Extra.fissureTicks--;
        if (Extra.tick % 15 == 0)
        {
            burst(owner, &Extra.fissure, owner, 6, 48, FX_SHAPE_DEBRIS, FX_COLOR_EMBER);
        }
    }
    statusTick(owner);
}

s32 gauntletBoonEffects_damage(u8 player, Actor *victim, s32 amount)
{
    if (!ensureEffects())
    {
        return amount;
    }
    const HackLibDamageSource *source = hackLib_currentDamageSource();
    BoonStatus *status = findStatus(victim, FALSE);
    bool skill = source != NULL && source->abilityRow < SKILL_EXPANSION_ACTIVE_ROW_COUNT;
    if (status != NULL)
    {
        Actor *owner = PLAYER(player).ownerActor;
        bool lit = HAS(LIGHT_OF_EARENDIL) && owner != NULL && victim != NULL &&
                   ABS((victim->coords.x - owner->coords.x) >> 16) +
                           ABS((victim->coords.y - owner->coords.y) >> 16) <=
                       72;
        if (status->dread && !lit)
        {
            amount = amount * 120 / 100;
        }
        if (status->frozen && HAS(SHATTER))
        {
            amount *= 2;
        }
    }
    if (victim != NULL && HAS(WARG_HUNTER) &&
        (victim->unitType == ENEMY_TYPE_WARG || victim->unitType == ENEMY_TYPE_WARG_ELITE ||
         (victim->unitType >= ENEMY_TYPE_SPIDER && victim->unitType <= ENEMY_TYPE_SPIDER_SMALL)))
    {
        amount = amount * 130 / 100;
    }
    bool primary = source != NULL && source->kind != HACKLIB_DAMAGE_SECONDARY;
    if (primary && skill && HAS(MEASURED_POWER))
    {
        amount = amount * 135 / 100;
    }
    if (primary && HAS(STEEL_OATH))
    {
        if (skill)
        {
            amount = amount * 75 / 100;
        }
        else if (source->kind == HACKLIB_DAMAGE_WEAPON || source->kind == HACKLIB_DAMAGE_PROJECTILE)
        {
            amount = amount * 140 / 100;
        }
    }
    if (skill && Extra.rowPower[source->abilityRow] != 0)
    {
        amount = amount * Extra.rowPower[source->abilityRow] / 2;
    }
    if (HAS(THE_ONE_RING) && Extra.idle >= 2 * SECOND)
    {
        amount *= 2;
    }
    (void)player;
    return amount > 32767 ? 32767 : amount;
}

static void tickBoonGhost(Actor *ghost)
{
    Actor *owner = ghost->parentActor;
    if (!gauntlet_isRunning() || owner != PLAYER(0).ownerActor || PLAYER(0).currentHp <= 0)
    {
        ghostState(ghost)->lifetime = 1;
    }
    HackLibDamageSource source = {ghost, HACKLIB_DAMAGE_SUMMON, HACKLIB_ABILITY_NONE,
                                  0,     HACKLIB_ELEMENT_NONE,  FALSE};
    const HackLibDamageSource *previous = hackLib_swapDamageSource(&source);
    hackLib_tickPersistentGhost(ghost, skillExpansion_isFoe, findNextEnemy);
    hackLib_swapDamageSource(previous);
}

static void summonBoonGhost(Actor *owner, Vector2Fp16 *position, Actor *target, u16 lifetime)
{
    u32 count = 0;
    Actor *cursor = NULL;
    while ((cursor = hackLib_nextOwnedGhost(owner, cursor)) != NULL)
    {
        count++;
    }
    if (count >= 4)
    {
        return;
    }
    if (HAS(HEIR_OF_ISILDUR))
    {
        lifetime = lifetime > 32767 ? 65535 : lifetime * 2;
    }
    Actor *ghost = spawnNpc(GHOST_NPC_KIND, position, owner, target, lifetime);
    if (ghost != NULL)
    {
        ghostState(ghost)->damage = 8;
        owner->as.combat.comboState[0]++;
        skillExpansion_trackGhost(ghost);
        ghost->tickCallback = tickBoonGhost;
    }
}

static void spreadOnDeath(Actor *owner, Actor *victim, BoonStatus saved, bool attackProcs)
{
    Vector2Fp16 center = victim->coords;
    if ((HAS(BLACK_SPEECH) || HAS(SPREADING_DREAD)))
    {
        dreadAround(owner, &center, HAS(SPREADING_DREAD) ? 80 : 64);
    }
    if (saved.venom && HAS(PLAGUE_SPREADER))
    {
        Actor *target = nearest(owner, &center, victim, 96);
        if (target != NULL)
        {
            venom(target, HAS(PANDEMIC) ? saved.venomStacks : (saved.venomStacks + 1) / 2);
        }
    }
    if (attackProcs && ((saved.frozen && HAS(SHATTER)) || (saved.burn && HAS(STORM_PYRE))))
    {
        burst(owner, &center, victim, 15, 48, FX_SHAPE_DIAMOND_RING,
              saved.frozen ? FX_COLOR_BLUE : FX_COLOR_EMBER);
        if (saved.frozen && HAS(PERMAFROST))
        {
            Actor *cursor = NULL;
            while ((cursor = hackLib_nextTargetAt(&center, victim, 48, cursor)) != NULL)
            {
                if (skillExpansion_isFoe(cursor))
                {
                    chill(cursor, 5);
                }
            }
        }
    }
}

static void handleEvent(u8 player, const HackLibCombatEvent *event)
{
    if (!ensureEffects())
    {
        return;
    }
    Actor *owner = PLAYER(player).ownerActor;
    Actor *victim = event->victim;
    BoonStatus *status = findStatus(victim, FALSE);
    BoonStatus saved = status != NULL ? *status : (BoonStatus){0};
    bool attackProcs = event->source.kind != HACKLIB_DAMAGE_SECONDARY;
    if (event->result.killed)
    {
        if (status != NULL)
        {
            *status = (BoonStatus){0};
        }
        spreadOnDeath(owner, victim, saved, attackProcs);
        if (saved.burn && HAS(WILDFIRE))
        {
            for (u32 row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
            {
                u16 *ticks = &skillExpansion_runtime[player].cooldowns[row];
                *ticks = *ticks > SECOND ? *ticks - SECOND : 0;
            }
        }
        if (attackProcs && HAS(OVERKILL) && event->result.overkillDamage > 0)
        {
            Actor *target = nearest(owner, &victim->coords, victim, 96);
            s32 amount = event->result.overkillDamage;
            if (HAS(REAPERS_DUE))
            {
                amount *= 2;
            }
            if (target != NULL)
            {
                damage(owner, target, amount, HACKLIB_DAMAGE_SECONDARY);
            }
        }
        bool summon = HAS(ARMY_OF_THE_DEAD);
        if (HAS(SUMMONERS_PACT) && ++Extra.summonKills >= 15)
        {
            Extra.summonKills = 0;
            summon = TRUE;
        }
        if (summon)
        {
            Actor *target = nearest(owner, &victim->coords, victim, 240);
            Vector2Fp16 position = victim->coords;
            summonBoonGhost(owner, &position, target, HAS(ARMY_OF_THE_DEAD) ? 1800 : 10 * SECOND);
        }
    }
    if (event->source.kind == HACKLIB_DAMAGE_PERIODIC ||
        event->source.kind == HACKLIB_DAMAGE_SECONDARY)
    {
        return;
    }
    if (!event->result.hit || event->result.damageDealt <= 0)
    {
        return;
    }
    Extra.idle = 0;
    bool skill = event->source.abilityRow < SKILL_EXPANSION_ACTIVE_ROW_COUNT;
    if (HAS(RAPID_FIRE))
    {
        Extra.rapid = Extra.rapid < 10 ? Extra.rapid + 1 : 10;
        Extra.rapidTicks = 3 * SECOND;
    }
    if (HAS(SIPHON) && skill)
    {
        s32 gain = PLAYER_ACTIVE_SKILL(player, event->source.abilityRow).spiritCost / 10;
        s32 cap = PLAYER_STAT(player, STAT_MAX_SPIRIT);
        if (HAS(OVERFLOW))
        {
            cap = cap * 3 / 2;
        }
        s32 spirit = PLAYER(player).currentSpirit + (gain > 0 ? gain : 1);
        PLAYER(player).currentSpirit = spirit > cap ? cap : spirit;
    }
    if (!event->result.killed)
    {
        if (HAS(KINDLING))
        {
            gauntletBoons_ignite(victim);
        }
        if (HAS(BARROW_VENOM))
        {
            venom(victim, 1);
        }
        if (HAS(CARADHRAS_CHILL))
        {
            chill(victim, 1);
        }
        if (HAS(ORC_CLEAVER))
        {
            status = findStatus(victim, TRUE);
            if (status != NULL)
            {
                status->bleed = 3 * SECOND;
            }
        }
        if (skill && HAS(ELEMENTAL_WEAVE))
        {
            if (Extra.weave == 1)
            {
                gauntletBoons_ignite(victim);
            }
            else if (Extra.weave == 2)
            {
                chill(victim, 1);
            }
            else
            {
                venom(victim, 1);
            }
        }
        if ((HAS(BOOMERANG) && event->source.kind == HACKLIB_DAMAGE_PROJECTILE) ||
            (HAS(VOICE_OF_COMMAND) && skill))
        {
            queueRepeat(victim, event->result.damageDealt / 2);
        }
    }
    if (skill && HAS(GRAVITY_WELL))
    {
        Actor *cursor = NULL;
        while ((cursor = hackLib_nextTargetAt(&victim->coords, victim, 56, cursor)) != NULL)
        {
            if (!skillExpansion_isFoe(cursor))
            {
                continue;
            }
            Vector2Fp16 position = cursor->coords;
            s32 dx = (victim->coords.x - cursor->coords.x) >> 16;
            s32 dy = (victim->coords.y - cursor->coords.y) >> 16;
            position.x += (dx < -8 ? -8 : dx > 8 ? 8 : dx) * 65536;
            position.y += (dy < -8 ? -8 : dy > 8 ? 8 : dy) * 65536;
            relocate(cursor, position);
        }
    }
    if (skill && HAS(ELEMENTAL_WEAVE) && Extra.weave == 0)
    {
        burst(owner, &victim->coords, victim, 15, 40, FX_SHAPE_TENDRIL_STAR, FX_COLOR_BLUE);
    }
    if (event->melee && HAS(BALROGS_WHIP))
    {
        if (!event->result.killed)
        {
            gauntletBoons_ignite(victim);
        }
        burst(owner, &victim->coords, victim, event->result.damageDealt / 2, 48,
              FX_SHAPE_RISING_STREAKS, FX_COLOR_EMBER);
    }
    if (HAS(ELVEN_VOLLEY) && event->source.kind == HACKLIB_DAMAGE_PROJECTILE)
    {
        Actor *cursor = NULL;
        u32 count = 0;
        while (count < 2 &&
               (cursor = hackLib_nextTargetAt(&victim->coords, victim, 48, cursor)) != NULL)
        {
            if (skillExpansion_isFoe(cursor))
            {
                damage(owner, cursor, event->result.damageDealt / 2, HACKLIB_DAMAGE_SECONDARY);
                count++;
            }
        }
    }
}

void gauntletBoonEffects_event(u8 player, const HackLibCombatEvent *event)
{
    if (!ensureEffects())
    {
        return;
    }
    bool previous = Extra.handlingEvent;
    Extra.handlingEvent = TRUE;
    handleEvent(player, event);
    Extra.handlingEvent = previous;
}

void gauntletBoons_castStarted(u8 player, u8 row, s32 before, s32 paid)
{
    if (!gauntlet_isRunning() || player != 0 || row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT)
    {
        return;
    }
    if (!ensureEffects())
    {
        return;
    }
    Extra.paidSpirit = paid > 65535 ? 65535 : paid > 0 ? paid : 0;
    Extra.previousRitualRow = Extra.ritualRow;
    Extra.previousRitual = Extra.ritual;
    Extra.previousWeave = Extra.weave;
    Extra.previousPower = Extra.rowPower[row];
    u8 power = 2;
    if (HAS(OVERFLOW) && before > PLAYER_STAT(player, STAT_MAX_SPIRIT))
    {
        power = 3;
    }
    if (HAS(RITUAL))
    {
        Extra.ritual = row == Extra.ritualRow ? Extra.ritual + 1 : 1;
        Extra.ritualRow = row;
        if (Extra.ritual >= 3)
        {
            power *= 3;
            Extra.ritual = 0;
        }
    }
    if (HAS(LAST_BREATH) && paid > 0 && PLAYER(player).currentSpirit < paid)
    {
        power *= 3;
    }
    Extra.rowPower[row] = power;
    Extra.weave = (Extra.weave + 1) % 3;
}

void gauntletBoons_castResolved(u8 player, u8 row, bool success)
{
    if (!gauntlet_isRunning() || player != 0 || row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT)
    {
        return;
    }
    if (!ensureEffects())
    {
        return;
    }
    if (!success)
    {
        if (!Extra.echoing)
        {
            Extra.paidSpirit = 0;
            Extra.ritualRow = Extra.previousRitualRow;
            Extra.ritual = Extra.previousRitual;
            Extra.weave = Extra.previousWeave;
            Extra.rowPower[row] = Extra.previousPower;
        }
        return;
    }
    bool herb = row == ACTIVE_SKILL_HERBAL_HEALING &&
                skillExpansion_getHeroKit(PLAYER(player).heroId) == NULL;
    if (!Extra.echoing)
    {
        if (!herb)
        {
            gauntletBoons_paidCast(player, Extra.paidSpirit);
            if (HAS(MEASURED_POWER))
            {
                u32 ticks = skillExpansion_runtime[player].cooldowns[row];
                if (ticks < SECOND)
                {
                    ticks = SECOND;
                }
                ticks += (ticks + 1) / 2;
                skillExpansion_runtime[player].cooldowns[row] = ticks > 65535 ? 65535 : ticks;
            }
        }
        Extra.paidSpirit = 0;
    }
    if (HAS(WRAITH_FORM))
    {
        Extra.immunity = SECOND;
    }
    if (HAS(MIRROR_IMAGE))
    {
        Extra.mirror = 2 * SECOND;
        hackLib_fxBurst(PLAYER(player).ownerActor, FX_SHAPE_SMOKE_SMALL, FX_COLOR_GHOST,
                        FX_LAYER_BEHIND);
    }
    Actor *owner = PLAYER(player).ownerActor;
    if (!herb && HAS(ECHO_CAST) && !Extra.echoing &&
        gauntlet_randBelow(100) < 20)
    {
        Extra.echoRow = row;
        Extra.echoTicks = SECOND / 2;
    }
    if (HAS(SPLIT_SHOT) && owner->facing < 8)
    {
        skillExpansion_launchProjectile(owner, (owner->facing * 4 + 30) % 32, 10, row);
        skillExpansion_launchProjectile(owner, (owner->facing * 4 + 2) % 32, 10, row);
        if (Extra.echoing && HAS(VOLLEY_ECHO))
        {
            skillExpansion_launchProjectile(owner, (owner->facing * 4 + 28) % 32, 10, row);
            skillExpansion_launchProjectile(owner, (owner->facing * 4 + 4) % 32, 10, row);
        }
    }
    if (HAS(TWIN_SOULS))
    {
        u8 weakest = 0;
        for (u32 i = 0; i < SKILL_EXPANSION_ACTIVE_ROW_COUNT; i++)
        {
            u8 level = *skillExpansion_activeLevel(player, i);
            if (level != 0 && i != row && (weakest == 0 || level < weakest))
            {
                weakest = level;
            }
        }
        Actor *target = nearest(owner, &owner->coords, owner, 128);
        if (target != NULL && weakest != 0)
        {
            damage(owner, target, 6 * weakest, HACKLIB_DAMAGE_SECONDARY);
        }
    }
    if (HAS(HEIR_OF_ISILDUR) && row == ACTIVE_SKILL_ARAGORN_CALL_OF_THE_DEAD)
    {
        Actor *ghost = NULL;
        while ((ghost = hackLib_nextOwnedGhost(owner, ghost)) != NULL)
        {
            if (ghostState(ghost)->lifetime == GHOST_LIFETIME_FRAMES)
            {
                ghostState(ghost)->lifetime = 2 * GHOST_LIFETIME_FRAMES;
            }
        }
    }
    if (HAS(FAULT_LINE) && row == ACTIVE_SKILL_GIMLI_EARTH_SHATTER)
    {
        Extra.fissure = owner->coords;
        Extra.fissureTicks = 3 * SECOND;
    }
    if (HAS(YOU_SHALL_NOT_PASS) && row == ACTIVE_SKILL_GANDALF_SHIELD)
    {
        Extra.reflect = 3 * SECOND;
    }
    if (HAS(MITHRANDIRS_STAFF))
    {
        for (Actor *cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
        {
            if (cursor->type == ACTOR_TYPE_PROJECTILE && cursor->parentActor != owner &&
                ABS((cursor->coords.x - owner->coords.x) >> 16) <= 96 &&
                ABS((cursor->coords.y - owner->coords.y) >> 16) <= 96)
            {
                cursor->flags.p |= ACTOR_FLAG_PENDING_REMOVE;
            }
        }
        burst(owner, &owner->coords, owner, 15, 64, FX_SHAPE_WIDE_RING, FX_COLOR_GOLD);
    }
}

void gauntletBoonEffects_hp(u8 player, s32 *amount, bool *flinch)
{
    if (!gauntlet_isRunning() || player != 0)
    {
        return;
    }
    if (!ensureEffects())
    {
        return;
    }
    if (*amount > 0 && HAS(WRAITH_FORM))
    {
        *amount /= 2;
    }
    if (*amount >= 0)
    {
        return;
    }
    if (HAS(STEADY_FOOTING))
    {
        *flinch = FALSE;
    }
    const HackLibDamageSource *source = hackLib_currentDamageSource();
    if (Extra.immunity || Extra.mirror)
    {
        Extra.mirror = 0;
        *amount = 0;
        return;
    }
    if (Extra.reflect && source != NULL && source->kind == HACKLIB_DAMAGE_PROJECTILE &&
        source->origin != NULL && skillExpansion_isFoe(source->origin))
    {
        damage(PLAYER(player).ownerActor, source->origin, 10, HACKLIB_DAMAGE_SECONDARY);
        *amount = 0;
        return;
    }
    if (HAS(PHALANX) && skillExpansion_runtime[player].pending && source != NULL &&
        source->origin != NULL)
    {
        Actor *owner = PLAYER(player).ownerActor;
        Vector2Int delta = {(source->origin->coords.x - owner->coords.x) >> 16,
                            (source->origin->coords.y - owner->coords.y) >> 16};
        u8 direction = math_vector2_direction8(delta);
        u8 difference = (direction + 8 - owner->facing) % 8;
        if (difference <= 1 || difference == 7)
        {
            *amount /= 2;
        }
    }
    if (source != NULL && source->origin != NULL && skillExpansion_isFoe(source->origin) &&
        HAS(IRON_HIDE) && HAS(THORNS) && ++Extra.thornHits >= 20)
    {
        Extra.thornHits = 0;
        gauntletBoons_rechargeSecondWind();
    }
}

u16 gauntletBoons_radius(Actor *owner, u16 radius)
{
    if (owner == PLAYER(0).ownerActor && HAS(LONG_REACH))
    {
        u32 widened = radius * 120 / 100;
        return widened > 32767 ? 32767 : widened;
    }
    return radius;
}
