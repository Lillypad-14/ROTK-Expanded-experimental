#include "lib/actor_ref.h"
#include "lib/pc_host.h"
#include "combat.h"
#include "enemy.h"
#include "memory.h"
#include "stats.h"
#include "variables.h"
#include <stdint.h>

static u32 *generations;
static u32 nextGeneration;

static s32 actorIndex(Actor *actor)
{
    uintptr_t address = (uintptr_t)actor;
    uintptr_t base = (uintptr_t)g_ActorPoolMain;
    u32 mainCapacity = hackLib_mainActorCapacity();
    uintptr_t offset;

    if (actor == NULL)
    {
        return -1;
    }
    if (base != 0 && address >= base && address < base + mainCapacity * sizeof(Actor))
    {
        offset = address - base;
        return offset % sizeof(Actor) == 0 ? (s32)(offset / sizeof(Actor)) : -1;
    }
    base = (uintptr_t)g_ActorPoolOverlay;
    if (base != 0 && address >= base &&
        address < base + hackLib_overlayActorCapacity() * sizeof(Actor))
    {
        offset = address - base;
        return offset % sizeof(Actor) == 0 ? (s32)(mainCapacity + offset / sizeof(Actor)) : -1;
    }
    return -1;
}

void hackLib_noteActorAllocated(Actor *actor)
{
    s32 index = actorIndex(actor);

    if (index < 0)
    {
        return;
    }
    if (generations == NULL)
    {
        generations = memory_zalloc((hackLib_mainActorCapacity() + hackLib_overlayActorCapacity()) *
                                    sizeof(u32));
        if (generations == NULL)
        {
            return;
        }
    }
    if (++nextGeneration == 0)
    {
        nextGeneration++;
    }
    generations[index] = nextGeneration;
}

void hackLib_noteActorRetired(Actor *actor)
{
    s32 index = actorIndex(actor);
    if (index >= 0 && generations != NULL)
    {
        generations[index] = 0;
    }
}

u32 hackLib_actorGeneration(Actor *actor)
{
    s32 index = actorIndex(actor);
    return index >= 0 && generations != NULL ? generations[index] : 0;
}

HackLibActorRef hackLib_actorRef(Actor *actor)
{
    return (HackLibActorRef){actor, hackLib_actorGeneration(actor)};
}

bool hackLib_isActorRefValid(HackLibActorRef reference)
{
    return reference.generation != 0 &&
           hackLib_actorGeneration(reference.actor) == reference.generation &&
           !(reference.actor->flags.p & ACTOR_FLAG_PENDING_REMOVE);
}

u8 g_HackLibPlayerProbe[HACKLIB_PROBE_COUNT][2];

__attribute__((weak)) u8 hackLib_beginActingFor(Actor *actor)
{
    (void)actor;
    return 0;
}

__attribute__((weak)) void hackLib_endActing(u8 previous)
{
    (void)previous;
}

void hackLib_probePlayer(u32 kind, const Actor *target)
{
    for (u32 player = 0; player < 2 && kind < HACKLIB_PROBE_COUNT; player++)
    {
        if (target != NULL && PLAYER(player).ownerActor == target)
        {
            g_HackLibPlayerProbe[kind][player]++;
        }
    }
}

s32 hackLib_actorMaxHp(const Actor *actor)
{
    if (actor == NULL)
    {
        return 0;
    }
    for (u32 player = 0; player < 2; player++)
    {
        if (PLAYER(player).ownerActor == actor)
        {
            return PLAYER_STAT(player, STAT_MAX_HP);
        }
    }
    return actor->unitType < ENEMY_TYPE_COUNT ? combat_getEnemyMaxHp(actor->unitType) : 0;
}

s32 hackLib_actorCurrentHp(const Actor *actor)
{
    if (actor == NULL)
    {
        return 0;
    }
    for (u32 player = 0; player < 2; player++)
    {
        if (PLAYER(player).ownerActor == actor)
        {
            return PLAYER(player).currentHp;
        }
    }
    return actor->currentHp;
}
