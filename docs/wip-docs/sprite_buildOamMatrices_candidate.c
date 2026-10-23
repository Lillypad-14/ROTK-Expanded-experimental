#include "sprite.h"
#include "variables.h"

/**
 * Build affine matrices in both shadow OAM banks from the active matrix parameters.
 *
 * @romaddress 0x08036660
 */
void sprite_buildOamMatrices(void)
{
    if (g_OamMatrixCount != 0)
    {
        bios_objAffineSet(g_OamMatrixParams, g_OamShadow + offsetof(OamEntry, affineParam),
                          g_OamMatrixCount, sizeof(OamEntry));
        bios_objAffineSet(g_OamMatrixParams,
                          g_OamShadow + OAM_ENTRY_COUNT * sizeof(OamEntry) + offsetof(OamEntry, affineParam),
                          g_OamMatrixCount, sizeof(OamEntry));
    }
}
