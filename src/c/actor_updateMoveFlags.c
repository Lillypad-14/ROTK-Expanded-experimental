#include "actor.h"
#include "match_hacks.h"
#include "variables.h"

/**
 * Track facing changes for a combat actor. Sets bit 0 of field_0x64 when the facing turns,
 * debounced by a 5 tick countdown in field_0x5e.
 *
 * @romaddress 0x0800a480
 */
void actor_updateMoveFlags(Actor *actor)
{
    u32 facing;
    MATCH_PIN(u32, state, "r4");
    MATCH_PIN(u32, prev, "r3");
    u32 lo;
    u32 hi;
    s16 *timer;

    facing = actor->facing;
    state = actor->actionState;
    prev = actor->field_0x66[1];
    lo = FacingNeighbors[facing][0];
    hi = FacingNeighbors[facing][1];

    if (*(u16 *)&actor->actionState == 0x1408)
    {
        timer = (s16 *)actor->field_0x5e;
        *timer -= 1;
        if (*timer <= 0)
        {
            if (facing != prev)
                actor->field_0x64[0] |= 1;
            actor->field_0x66[1] = facing;
            *timer = 5;
        }
        else if (facing != prev)
            actor->field_0x64[0] &= ~1;
    }
    else if (state == 8 || state == 2 || state == 9)
    {
        s16 *tp = (s16 *)actor->field_0x5e;
        *tp -= 1;
        timer = tp;
        if ((prev == facing || prev == lo || prev == hi) && *timer > 0)
            return;
        if (prev != facing)
            actor->field_0x64[0] |= 1;
        actor->field_0x66[1] = actor->facing;
        *timer = 5;
    }
    else if (prev != facing)
        actor->field_0x66[1] = facing;
}
