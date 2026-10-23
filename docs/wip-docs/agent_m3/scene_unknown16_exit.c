#include "gfx.h"

/**
 * Fade out to black while the scene is still running.
 *
 * @romaddress 0x080304d4
 */
void scene_unknown16_exit(void)
{
    gfx_fadeOutToBlackWhileRunning(0x15);
}
