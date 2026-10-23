#include "actor.h"

/**
 * @romaddress 0x08045584
 */
u32 actor_unknown08045584(Actor *actor)
{
    if (--actor->actionStateTimer <= 0)
        actor->actionState = ACTOR_STATE_UNKNOWN_04;
    if (actor->actionState == ACTOR_STATE_UNKNOWN_04)
    {
        actor->flags.p &= ~ACTOR_FLAG_RENDER;
        actor->flags.p |= ACTOR_FLAG_ANIMATION_FRAME_CHANGED | ACTOR_FLAG_PENDING_REMOVE;
        return TRUE;
    }
    return FALSE;
}
