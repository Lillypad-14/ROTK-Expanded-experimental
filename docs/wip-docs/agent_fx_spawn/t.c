#include "actor.h"
#include "fx.h"
#include "sprite.h"

typedef struct FxDesc
{
    u8 anim[0x10];
    u8 flags;
    s8 offX;
    u8 offY;
    u8 pad;
} FxDesc;

extern FxDesc g_FxDescTable[];
extern void actor_unk0800103c(Actor *a, u8 b, Vector2Fp16 *out, s8 c);

Actor *fx_spawnOnActor(Actor *parent, u8 fxType, u8 variant, u8 alt)
{
    Vector2Fp16 pos;
    Actor *fx = actor_allocMain();
    FxDesc *d;
    FxDesc *d2;
    FxDesc *tbl = g_FxDescTable;

    if (fx == NULL)
    {
        return NULL;
    }
    d = &tbl[fxType];
    if (d->flags & 4)
    {
        actor_getAnimationFrame(parent, (u32 *)&pos);
        pos.x = parent->coords.x;
        pos.y = parent->coords.y - (pos.y << 15);
    }
    else if (d->flags & 2)
    {
        actor_unk0800103c(parent, d->offY, &pos, d->offX);
    }
    else
    {
        pos.y = parent->coords.y;
        pos.x = parent->coords.x;
    }
    *(s32 *)((u8 *)&fx->as + 0x20) = fx->coords.x = fx->renderCoords.x = pos.x;
    *(s32 *)((u8 *)&fx->as + 0x24) = fx->coords.y = fx->renderCoords.y = pos.y;
    fx->velocity.x = 0;
    fx->velocity.y = 0;
    fx->parentActor = parent;
    d2 = &g_FxDescTable[fxType];
    sprite_setAnimation(fx, (SpriteAnimation *)d2);
    fx->oam.paletteNumber = variant;
    fx->oam.bgPriority = parent->oam.bgPriority;
    fx->oam.colors256 = 0;
    fx->oam.objMode = 0;
    fx->flags.p = 0x10019;
    if (d2->flags & 2) goto A;
    if (d2->flags & 4)
    {
    A:
        fx->flags.p |= (alt == 0 ? 0x100000 : 0x80000);
    }
    else
    {
        if (alt == 0)
            fx->spriteFlags = 8;
        else
            fx->spriteFlags = 4;
        fx->renderFollowTarget = parent;
    }
    fx->as.shieldGandalf.followTarget = parent;
    fx->as.combat.appliedPose = fxType;
    fx->actionState = ACTOR_STATE_FX_ATTACHED;
    return fx;
}
