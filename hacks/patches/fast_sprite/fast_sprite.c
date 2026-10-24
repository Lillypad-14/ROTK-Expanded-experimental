#if HACK_fast_sprite

#include "fast_sprite.h"
#include "variables.h"

void fastSprite_composeFrame(const SpriteFrameSetHeader *frames, u16 frameIndex, u8 flags,
                             const Vector2Int *position, u16 tileNumber, const OamEntryWords *base,
                             Actor *actor)
{
    u32 firstTile;
    u32 count;
    s32 scaleX;
    u32 hide;
    OamEntryWords entry;
    Vector2Int dimensions;
    s32 scaleY = (scaleX = actor->sprite.invScaleX << 8, actor->sprite.invScaleY << 8);
    u32 index = frameIndex << 1;
    const u8 *offsets = (const u8 *)(frames + 1);
    u16 offset = *(const u16 *)(offsets + index);
    const SpriteFrame *frame =
        (const SpriteFrame *)((const u8 *)frames + (offset + sizeof(SpriteFrameSetHeader)));
    u32 frameFlags = frame->flags.p;
    const u8 *boxes =
        (count = 31, (const u8 *)frame) +
        (sizeof(SpriteFrame) + frames->triggerBoxCount * sizeof(struct ActorTriggerBox));
    const union SpriteOamTemplate *pieces =
        (const union SpriteOamTemplate *)(boxes + frames->hotspotCount * sizeof(SpriteHotspot));
    firstTile = tileNumber;
    count &= frameFlags;
    if (count == 0)
    {
        return;
    }
    do
    {
        s32 x;
        s32 y;
        s32 relative;
        u32 affineMode;
        entry = *base;
        entry.entry.tileNumber = firstTile + pieces->d.tileOffset;
        entry.entry.objSize = pieces->d.objSize;
        entry.entry.objShape = pieces->d.objShape;
        if ((entry.entry.affineMode & 1) ||
            (entry.entry.matrixIndex & (OAM_MATRIX_H_FLIP | OAM_MATRIX_V_FLIP)) ||
            ((flags & 6) == 2))
        {
            sprite_getOamShapeDims(entry.entry.objShape, entry.entry.objSize, &dimensions);
        }
        affineMode = entry.entry.affineMode;
        if (affineMode == OAM_AFFINE_DOUBLE)
        {
            relative =
                (pieces->d.x * scaleX - (((dimensions.x << 19) - dimensions.x * scaleX) >> 1)) >>
                18;
            x = position->x + relative;
        }
        else if (affineMode == OAM_AFFINE_ON)
        {
            relative = ((pieces->d.x * scaleX) >> 18) -
                       (dimensions.x - ((dimensions.x * scaleX) >> 18) + 1) / 2;
            x = position->x + relative;
        }
        else if (((entry.words[0] << 3) >> 31) == 1)
        {
            x = position->x - pieces->d.x - dimensions.x;
        }
        else
        {
            x = position->x + pieces->d.x;
        }
        affineMode = (entry.words[0] << 22) >> 30;
        if (affineMode == OAM_AFFINE_DOUBLE)
        {
            relative = ((pieces->d.y * scaleY) >> 18) -
                       ((dimensions.y * 2 - ((dimensions.y * scaleY) >> 18)) / 2);
            y = position->y + relative;
        }
        else if (affineMode == OAM_AFFINE_ON)
        {
            y = position->y + (((pieces->d.y * scaleY) >> 18) -
                               (dimensions.y - ((dimensions.y * scaleY) >> 18)) / 2);
        }
        else if ((s32)(entry.words[0] << 2) < 0)
        {
            y = position->y - pieces->d.y - dimensions.y;
        }
        else if (flags & 2)
        {
            if (flags & 4)
            {
                y = position->y + pieces->d.y + 10;
            }
            else
            {
                y = position->y - pieces->d.y - dimensions.y;
                entry.words[0] |= (u32)OAM_MATRIX_V_FLIP << 25;
            }
            if ((g_GameFlags.p & (GAME_FLAG_25 | GAME_FLAG_16)) != GAME_FLAG_25)
            {
                entry.entry.bgPriority = 3;
            }
            else
            {
                entry.entry.bgPriority = 2;
            }
        }
        else
        {
            y = position->y + pieces->d.y;
        }
        pieces++;
        if ((u32)(y + 64) <= 256 && x + 64 >= 0 && x <= 255)
        {
            entry.entry.y = y;
            x &= 0x1ff;
            entry.entry.x = x;
            if ((g_GameFlags.p & GAME_FLAG_25) && entry.entry.bgPriority != 0 &&
                entry.entry.bgPriority != 1 && !(flags & 2))
            {
                entry.entry.bgPriority = (u16)(entry.entry.bgPriority - 1);
            }
            hide = flags & 3;
            sprite_pushOam(g_SpriteAllocCount, &entry);
            if (hide)
            {
                sprite_hideOamEntry(g_SpriteAllocCount - 1);
            }
        }
        count--;
    } while (count != 0);
}

#endif
