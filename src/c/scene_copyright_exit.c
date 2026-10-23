#include "gfx.h"
#include "scene/copyright.h"

/**
 * Fade out the copyright screen.
 *
 * @romaddress 0x0802ff68
 */
void scene_copyright_exit(void)
{
    gfx_fadeOutToBlack(0x3f);
}
