#include "actor.h"
#include "actor/enemySpawner.h"

/**
 * Find the first empty child slot of an enemy spawner.
 *
 * @romaddress 0x080220c0
 */
u32 actor_enemySpawner_findFreeChildSlot(Actor *actor)
{
    u8 i;

    for (i = 0; i <= 3; i++)
    {
        if (actor->as.enemySpawner.children[i] == NULL)
            return i;
    }
    return 4;
}
