#include "actor.h"
#include "actor/enemySpawner.h"
#include "rng.h"

// The unused RNG spawner variant stores its spawn ids at 0x94/0x96/0x98 and two
// probabilities at 0x9a/0x9b of the actor.
#define SPAWNER_BYTE(actor, offset) ((u8 *)(actor) + (offset))

/**
 * Spawn one child into a free slot of an enemy spawner.
 *
 * @return TRUE when a child was spawned
 *
 * @romaddress 0x08021e30
 */
bool actor_enemySpawner_trySpawn(Actor *actor)
{
    u8 *idPtr;
    u8 roll;
    u8 threshold;
    u8 enemySpawnId;
    u8 slot;
    Actor *child;

    if (actor->type == ACTOR_TYPE_ENEMY_SPAWNER)
    {
        idPtr = SPAWNER_BYTE(actor, 0x94);
    }
    else
    {
        roll = rng_mod(100);
        threshold = 100 - *SPAWNER_BYTE(actor, 0x9a);
        if (roll > threshold)
        {
            idPtr = SPAWNER_BYTE(actor, 0x94);
        }
        else if (roll > *SPAWNER_BYTE(actor, 0x9b) && roll < threshold)
        {
            idPtr = SPAWNER_BYTE(actor, 0x96);
        }
        else
        {
            idPtr = SPAWNER_BYTE(actor, 0x98);
        }
    }
    enemySpawnId = *idPtr;
    slot = actor_enemySpawner_findFreeChildSlot(actor);
    if (slot == 4)
    {
        return FALSE;
    }
    child = actor_enemySpawner_spawnChild(enemySpawnId, actor);
    if (child == NULL)
    {
        return FALSE;
    }
    actor->as.enemySpawner.children[slot] = child;
    if (!actor->as.enemySpawner.unbudgeted)
    {
        actor->as.enemySpawner.budget--;
    }
    actor->as.enemySpawner.cap--;
    return TRUE;
}
