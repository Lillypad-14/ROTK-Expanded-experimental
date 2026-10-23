#include "actor.h"
#include "math.h"
#include "sprite.h"

typedef struct Osc
{
    u16 phaseX;
    u16 phaseY;
    s16 stepX;
    s16 stepY;
    s16 amplitudeX;
    s16 amplitudeY;
} Osc;

void actor_applyRenderOscillation(Actor *actor)
{
    Osc *o = (Osc *)((u8 *)actor + 0x44);

    if (o->stepX != 0)
    {
        o->phaseX += o->stepX;
        actor->renderCoords.x += o->amplitudeX * math_cosFp16(o->phaseX >> 8);
    }
    if (o->stepY != 0)
    {
        o->phaseY += o->stepY;
        actor->renderCoords.y += o->amplitudeY * math_sinFp16(o->phaseY >> 8);
    }
}
