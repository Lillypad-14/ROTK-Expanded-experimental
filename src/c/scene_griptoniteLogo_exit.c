#include "gfx.h"
#include "scene/griptoniteLogo.h"

/**
 * Fade out the logo screen and stop its palette animations.
 *
 * @romaddress 0x0801a928
 */
void scene_griptoniteLogo_exit(void)
{
    gfx_fadeOutToBlackWhileRunning(0x3f);
    gfx_clearPaletteAnims();
}
