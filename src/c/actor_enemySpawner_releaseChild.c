#include "actor.h"
#include "actor/enemySpawner.h"

/**
 * @romaddress 0x08022088
 */
void actor_enemySpawner_releaseChild(Actor *child)
{
    Actor *spawner = child->parentActor;
    u8 i;

    for (i = 0; i < 4; i++)
    {
        if (spawner->as.enemySpawner.children[i] == child)
        {
            spawner->as.enemySpawner.children[i] = NULL;
            spawner->as.enemySpawner.cap++;
            i = 4;
        }
    }
}
