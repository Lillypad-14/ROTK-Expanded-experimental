#include "actor.h"
#include "actor/enemySpawner.h"
#include "rng.h"

Actor *actor_enemySpawner_spawnChild(u16 enemySpawnId, Actor *spawner);

/**
 * Spawn one child into a free slot, decrementing the budget.
 *
 * @return TRUE when a child was spawned
 *
 * @romaddress 0x08021e30
 */
bool actor_enemySpawner_trySpawn(Actor *actor)
{
    u8 *src;
    u8 id;
    u8 slot;
    Actor *child;

    if (actor->type == ACTOR_TYPE_ENEMY_SPAWNER)
    {
        src = &actor->as.combat.appliedPose;
    }
    else
    {
        u8 roll = rng_mod(100);
        u8 hi = 100 - actor->as.combat.comboState[0];

        if (roll > hi)
        {
            src = &actor->as.combat.appliedPose;
        }
        else if (roll > actor->as.combat.comboState[1] && roll < hi)
        {
            src = &actor->as.combat.playerIndex;
        }
        else
        {
            src = actor->as.combat.field_0x97 + 1;
        }
    }
    id = *src;
    slot = actor_enemySpawner_findFreeChildSlot(actor);
    if (slot == 4 || (child = actor_enemySpawner_spawnChild(id, actor)) == NULL)
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
