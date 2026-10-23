#include "actor.h"
#include "fx.h"
#include "sfx.h"
#include "variables.h"

extern u32 g_StunSfxHandle;
extern s16 g_StunSfxCooldown;
void fx_statusEffect_tick(Actor *fx);

Actor *fx_attachStatusEffect(Actor *parent, Actor *target, u16 duration, u8 kind, u8 arg4, u8 arg5,
                             u8 flagBit)
{
    Actor *fx;
    u8 tagged = target->field_0x100 & flagBit;

    if (tagged || !(target->flags.p & ACTOR_FLAG_ON_SCREEN))
    {
        return NULL;
    }
    {
        if (kind == 0xb)
        {
            fx = fx_spawnTimedOnActor(target, (s16)duration, 0xb, arg4, tagged);
        }
        else
        {
            fx = fx_spawnTimedOnActor(target, (s16)duration, kind, arg4, 1);
        }
        if (fx != NULL)
        {
            fx->flags.p |= ACTOR_FLAG_TICK_ALWAYS;
            if (arg5)
            {
                fx->spriteFlags |= 0x20;
            }
            fx->type = 0x68;
            if (kind == 0xb)
            {
                fx->spriteCellIndex = 0xd;
                if (g_StunSfxCooldown <= 0)
                {
                    g_StunSfxHandle = sfx_playUnlessPlaying(0x8e, g_StunSfxHandle);
                    g_StunSfxCooldown = 450;
                }
            }
            else
            {
                fx->spriteCellIndex = 0xe;
            }
            fx->tickCallback = fx_statusEffect_tick;
            fx->triggerCallbacks[0] = NULL;
            fx->parentActor = parent;
            target->field_0x100 |= flagBit;
            *(u16 *)((u8 *)fx + 0x8e) = flagBit;
            return fx;
        }
    }
    return NULL;
}
