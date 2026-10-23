#include "actor.h"
#include "fx.h"
#include "variables.h"

/**
 * Per-frame tick of the status-effect FX actor spawned by fx_attachStatusEffect.
 *
 * @romaddress 0x0801c44c
 */
void fx_statusEffect_tick(Actor *fx)
{
    Actor *target = fx->as.combat.head.attachTarget;

    if (fx->parentActor != NULL)
    {
        u8 state = fx->parentActor->actionState;
        if (state == 0x2d || state == 0x11 || state == 4)
        {
            fx->actionState = 4;
        }
    }
    if (*(u8 *)&fx->as.combat.fearDuration == 0xb)
    {
        if (!g_FlagBearerCount || !(fx->flags.p & ACTOR_FLAG_ON_SCREEN))
        {
            fx->actionState = 4;
        }
    }
    switch ((s16)fx->as.combat.storedDamage)
    {
    case 8:
        if (fx->renderFollowTarget->actionState != ACTOR_STATE_STUNNED)
        {
            goto set;
        }
    default:
        break;
    }
    if ((s16)fx->as.combat.storedDamage == 2)
    {
        Actor *owner = fx->renderFollowTarget;
        if (((u8 *)owner)[0x69] & 0x20)
            fx->as.combat.venomDuration = *(u16 *)((u8 *)owner + 0x54);
        else
        {
        set:
            fx->actionState = 4;
        }
    }
    if (fx_tickTimedAttachment(fx) == 1)
    {
        fx->flags.p |= 0x82;
        target->field_0x100 &= ~(u8)fx->as.combat.storedDamage;
    }
}
