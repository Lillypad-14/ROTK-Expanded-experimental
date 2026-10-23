#include "actor.h"
#include "fx.h"
#include "sprite.h"

extern u8 g_FxDescTable[];
extern void actor_unk0800103c(Actor *a, u8 b, Vector2Fp16 *out, s8 c);

Actor *fx_spawnOnActor(Actor *parent, u8 fxType, u8 variant, u8 alt)
{
    Vector2Fp16 pos;
    Actor *fx = actor_allocMain();
    u8 *row;

    if (fx == NULL)
    {
        return NULL;
    }
    row = g_FxDescTable + fxType * 0x14;
    if (row[0x10] & 4)
    {
        actor_getAnimationFrame(parent, (u32 *)&pos);
        pos.x = parent->coords.x;
        pos.y = parent->coords.y - (pos.y << 15);
    }
    else if (row[0x10] & 2)
    {
        actor_unk0800103c(parent, row[0x12], &pos, (s8)row[0x11]);
    }
    else
    {
        pos.y = parent->coords.y;
        pos.x = parent->coords.x;
    }
    fx->renderCoords.x = fx->coords.x = *(s32 *)((u8 *)&fx->as + 0x20) = pos.x;
    fx->renderCoords.y = fx->coords.y = *(s32 *)((u8 *)&fx->as + 0x24) = pos.y;
    fx->velocity.x = 0;
    fx->velocity.y = 0;
    fx->parentActor = parent;
    sprite_setAnimation(fx, (SpriteAnimation *)(g_FxDescTable + fxType * 0x14));
    fx->oam.paletteNumber = variant;
    fx->oam.bgPriority = parent->oam.bgPriority;
    fx->oam.objMode = 0;
    fx->oam.colors256 = 0;
    fx->flags.p = 0x10019;
    if (!(g_FxDescTable[fxType * 0x14 + 0x10] & 2) && !(g_FxDescTable[fxType * 0x14 + 0x10] & 4))
    {
        if (alt == 0)
            fx->spriteFlags = 8;
        else
            fx->spriteFlags = 4;
        fx->renderFollowTarget = parent;
    }
    else
    {
        fx->flags.p = (alt == 0 ? 0x100000 : 0x80000) | 0x10019;
    }
    fx->as.shieldGandalf.followTarget = parent;
    fx->as.combat.appliedPose = fxType;
    fx->actionState = ACTOR_STATE_FX_ATTACHED;
    return fx;
}
