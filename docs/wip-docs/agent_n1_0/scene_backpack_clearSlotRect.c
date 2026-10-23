#define scene_backpack_clearSlotRect hdr_dummy
#include "gfx.h"
#include "scene/backpack.h"
#undef scene_backpack_clearSlotRect

void unk_08012648(s32 layer, s32 col, s32 row, s32 width, s32 height);
asm(".global unk_08012648\n.thumb_set unk_08012648, 0x08012648+1");
extern const s32 BackpackCellPositions[][2];
extern const s32 BackpackCellSizes[][2];
asm(".global BackpackCellPositions\n.set BackpackCellPositions, 0x08062aa4");
asm(".global BackpackCellSizes\n.set BackpackCellSizes, 0x08062c24");

/**
 * Clear the BG2/BG3 tile rect behind an equipment cell's icon.
 *
 * @romaddress 0x0802e804
 */
void scene_backpack_clearSlotRect(u8 cell)
{
    s32 x = BackpackCellPositions[cell][0];
    s32 col = x / 8;
    s32 y = BackpackCellPositions[cell][1];
    s32 row = y / 8;
    s32 width = BackpackCellSizes[cell][0];
    s32 height = BackpackCellSizes[cell][1];

    unk_08012648(2, col, row, width, height);
    unk_08012648(3, col, row, width, height);
}
