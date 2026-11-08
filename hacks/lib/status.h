#pragma once

#include "actor.h"
#include "lib/actor_ref.h"
#include "fx.h"

/** Apply or extend a stun. Callers validate enemy allegiance and hit eligibility. */
static inline bool hackLib_applyStun(Actor *actor, s32 frames)
{
    if (actor == NULL || hackLib_actorCurrentHp(actor) <= 0 || frames <= 0 || !actor_isStunnable(actor))
    {
        return FALSE;
    }
    if (frames > 0xffff)
    {
        frames = 0xffff;
    }
    if (actor->actionState == ACTOR_STATE_STUNNED && actor->actionStateTimer >= frames)
    {
        return FALSE;
    }
    actor->actionState = ACTOR_STATE_STUNNED;
    actor->actionStateTimer = frames;
    hackLib_probePlayer(HACKLIB_PROBE_STUN, actor);
    actor->field_0x64[0] |= 2;
    fx_attachStatusEffect(NULL, actor, frames, 0xc, 8, 0, 8);
    return TRUE;
}
