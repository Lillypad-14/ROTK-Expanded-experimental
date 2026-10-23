#include "math.h"
#include "sprite.h"

/**
 * Advance render-only oscillation phases and offset the actor's render position.
 *
 * @romaddress 0x08003650
 */
void actor_applyRenderOscillation(Actor *actor)
{
    if (ACTOR_TRIGGER_SCRATCH(actor)->oscillation.stepX != 0)
    {
        ACTOR_TRIGGER_SCRATCH(actor)->oscillation.phaseX +=
            ACTOR_TRIGGER_SCRATCH(actor)->oscillation.stepX;
        actor->renderCoords.x +=
            ACTOR_TRIGGER_SCRATCH(actor)->oscillation.amplitudeX *
            math_cosFp16(ACTOR_TRIGGER_SCRATCH(actor)->oscillation.phaseX >> 8);
    }
    if (ACTOR_TRIGGER_SCRATCH(actor)->oscillation.stepY != 0)
    {
        ACTOR_TRIGGER_SCRATCH(actor)->oscillation.phaseY +=
            ACTOR_TRIGGER_SCRATCH(actor)->oscillation.stepY;
        actor->renderCoords.y +=
            ACTOR_TRIGGER_SCRATCH(actor)->oscillation.amplitudeY *
            math_sinFp16(ACTOR_TRIGGER_SCRATCH(actor)->oscillation.phaseY >> 8);
    }
}
