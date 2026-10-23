#include "actor.h"

bool actor_isStunnable(Actor *actor)
{
    u8 state = actor->actionState;

    if (state != ACTOR_STATE_UNKNOWN_11 && state != ACTOR_STATE_UNKNOWN_15 &&
        state != ACTOR_STATE_UNKNOWN_04 && state != 0x1b && state != 0x13 &&
        actor->behaviorState != 0x2f && !(actor->flags.p & ACTOR_FLAG_PENDING_REMOVE))
    {
        return TRUE;
    }
    return FALSE;
}
