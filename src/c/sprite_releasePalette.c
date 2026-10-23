#include "sprite.h"
#include "variables.h"

/**
 * Release an actor's palette reference and defer reclaiming an unused dynamic palette.
 *
 * @romaddress 0x080382a8
 */
void sprite_releasePalette(Actor *actor)
{
    if (actor->flags.p & ACTOR_FLAG_DEFAULTS_APPLIED)
    {
        g_ObjPaletteRefCounts[actor->oam.paletteNumber]--;
        if (actor->oam.paletteNumber > 8 && g_ObjPaletteRefCounts[actor->oam.paletteNumber] == 0)
        {
            g_ObjPaletteReleaseTimers[actor->oam.paletteNumber] = SPRITE_PALETTE_RELEASE_DELAY;
        }
    }
}
