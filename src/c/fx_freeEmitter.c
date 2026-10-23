#include "actor.h"
#include "fx.h"
#include "variables.h"

/**
 * Return an emitter to the emitter pool.
 *
 * @romaddress 0x08039138
 */
void fx_freeEmitter(FxEmitter *emitter)
{
    pool_free(&g_FxEmitterFreeList, (ActorList *)&g_FxEmitterListHead, (Actor *)emitter);
    g_FxEmitterCount--;
}
