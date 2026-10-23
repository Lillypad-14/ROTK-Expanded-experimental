#include "actor.h"
#include "fx.h"
#include "variables.h"
extern u8 g_FlagBearerCount;
void fx_statusEffect_tick(Actor *fx);



void fx_statusEffect_tick(Actor *fx)
{
    Actor *target = fx->as.combat.head.attachTarget;
    u8 *kind = (u8 *)&fx->as.combat.fearDuration;
    s16 *tag = (s16 *)&fx->as.combat.storedDamage;

    if (fx->parentActor != NULL)
    {
        u8 state = fx->parentActor->actionState;
        if (state == 0x2d || state == 0x11 || state == 4)
        {
            fx->actionState = 4;
        }
    }
    if (*kind == 0xb)
    {
        if (!g_FlagBearerCount || !(fx->flags.p & ACTOR_FLAG_ON_SCREEN))
        {
            fx->actionState = 4;
        }
    }
    if (*tag == 8 && fx->renderFollowTarget->actionState != ACTOR_STATE_STUNNED)
    {
        fx->actionState = 4;
    }
    else if (*tag == 2)
    {
        Actor *owner = fx->renderFollowTarget;
        if (!(((u8 *)owner)[0x69] & 0x20))
        {
            fx->actionState = 4;
        }
        else
        {
            fx->as.combat.venomDuration = *(u16 *)((u8 *)owner + 0x54);
        }
    }
    if (fx_tickTimedAttachment(fx) == 1)
    {
        fx->flags.p |= 0x82;
        target->field_0x100 &= ~*(u8 *)tag;
    }
}
