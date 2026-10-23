#include "actor.h"
#include "fx.h"
#include "variables.h"

/**
 * Free every live emitter.
 *
 * @romaddress 0x0803915c
 */
void mission_unknown0803915c(void)
{
    Actor *node;
    Actor *next;

    if (g_FxEmitterCount != 0)
    {
        node = g_FxEmitterListHead;
        if (node != NULL)
        {
            do
            {
                next = *(Actor **)node;
                pool_free(&g_FxEmitterFreeList, (ActorList *)&g_FxEmitterListHead, node);
                g_FxEmitterCount--;
                node = next;
            } while (node != NULL);
        }
    }
}
