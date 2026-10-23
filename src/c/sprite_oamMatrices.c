#include "match_hacks.h"
#include "sprite.h"
#include "variables.h"

/**
 * Read an OAM entry's affine matrix index.
 *
 * @romaddress 0x080365b4
 */
u16 sprite_getOamMatrixIndex(OamEntry *oam)
{
    OamEntryWords *words = (OamEntryWords *)oam;

    return (words->halfwords[1] >> 9) & 0x1f;
}

/**
 * Set an OAM entry's affine matrix index.
 *
 * @romaddress 0x080365c0
 */
void sprite_setOamMatrixIndex(OamEntry *oam, u16 index)
{
    OamEntryWords *words = (OamEntryWords *)oam;

    words->halfwords[1] = (words->halfwords[1] & 0xc1ff) | (index << 9);
}

/**
 * Reserve the first available OBJ affine matrix.
 *
 * @return OAM_MATRIX_COUNT when all matrices are allocated.
 *
 * @romaddress 0x080365d4
 */
u16 sprite_allocOamMatrix(void)
{
    u16 index = 0;
    u16 remaining = OAM_MATRIX_COUNT;
    u8 *allocationFlags = g_OamMatrixAllocFlags;
    // HACK: preserve the original count-address register through the search loop.
    MATCH_PIN(u8 *, count, "r4") = &g_OamMatrixCount;

    do
    {
        u8 *allocated = (u8 *)(index + (u32)allocationFlags);
        if (*allocated != 1)
        {
            *allocated = 1;
            if (index >= *count)
            {
                (*count)++;
            }
            return index;
        }
        index++;
        remaining--;
    } while (remaining != 0);
    return index;
}

/**
 * Release an OBJ affine matrix and trim trailing free slots from the active range.
 *
 * @romaddress 0x08036618
 */
void sprite_freeOamMatrix(u16 index)
{
    g_OamMatrixAllocFlags[index] = 0;
    if (index == g_OamMatrixCount - 1)
    {
        while (g_OamMatrixCount != 0)
        {
            g_OamMatrixCount--;
            index--;
            if (g_OamMatrixCount == 0 || g_OamMatrixAllocFlags[index] != 0)
            {
                break;
            }
        }
    }
}
