#include "match_hacks.h"
#include "sprite.h"
#include "variables.h"

static inline const SpriteFrameSetHeader *getSlotFrames(const SpriteAnimation *const *bank,
                                                        s32 part, s32 pose, s32 facing)
{
    MATCH_PIN(const SpriteAnimation *const *, address, "r1");
    s32 index;
    address = bank;
    index = part * 7 + pose;
    address += index;
    return (*address + facing)->frames;
}

static inline u32 usesPreviousFrame(u32 flags)
{
    u32 previous = 8;
    previous &= flags;
    return previous;
}

/**
 * Commit changed actor and attachment frames to OBJ tile memory.
 *
 * @romaddress 0x0800304c
 */
void sprite_commitGfx(Actor *actor, bool commit)
{
    const void *tiles;
    u32 submit = commit;
    u32 dimensions[2];
    u32 tileNumber;
    u32 tileBytes;
    u32 upload = 1;
    u32 currentFlags = actor->flags.p;
    u8 spriteFlags = actor->spriteFlags;
    u32 i;
    if (!(currentFlags & ACTOR_FLAG_ANIMATION_FRAME_CHANGED) && actor->tileNumber != 0xffff)
    {
        goto done;
    }
    if (submit == TRUE &&
        (currentFlags & (ACTOR_FLAG_ON_ACTIVE_PLAYER_SCREEN | ACTOR_FLAG_FREEZE_GFX |
                         ACTOR_FLAG_PENDING_REMOVE)) == ACTOR_FLAG_ON_ACTIVE_PLAYER_SCREEN)
    {
        tileBytes = actor_getAnimationFrame(actor, dimensions);
        if (spriteFlags & 3)
        {
            if (spriteFlags & 1)
            {
                if (g_SpriteCellCache[actor->spriteCellIndex].refCounts[0] == 0)
                {
                    g_SpriteCellCache[actor->spriteCellIndex].owner = actor;
                    if (g_SpriteCellCache[actor->spriteCellIndex].tileNumbers[0] != 0xffff)
                    {
                        sprite_freeObjTilesDeferred(
                            g_SpriteCellCache[actor->spriteCellIndex].tileNumbers[0],
                            g_SpriteCellCache[actor->spriteCellIndex].gfxSize);
                        actor->tileNumber = 0xffff;
                    }
                    g_SpriteCellCache[actor->spriteCellIndex].gfxSize = actor_getAnimationFrame(
                        g_SpriteCellCache[actor->spriteCellIndex].owner, dimensions);
                    tileBytes = g_SpriteCellCache[actor->spriteCellIndex].gfxSize;
                    g_SpriteCellCache[actor->spriteCellIndex].refCounts[0]++;
                    tileNumber = sprite_allocObjTiles(NULL, tileBytes);
                    g_SpriteCellCache[actor->spriteCellIndex].tileNumbers[0] = tileNumber;
                }
                else
                {
                    g_SpriteCellCache[actor->spriteCellIndex].refCounts[0]++;
                    tileBytes = g_SpriteCellCache[actor->spriteCellIndex].gfxSize;
                    tileNumber = g_SpriteCellCache[actor->spriteCellIndex].tileNumbers[0];
                    upload = 0;
                    actor->tileNumber = 0xffff;
                }
            }
            else
            {
                u32 frame = actor->currentAnimationFrame;
                if (g_SpriteCellCache[actor->spriteCellIndex].refCounts[frame] == 0)
                {
                    tileNumber = sprite_allocObjTiles(NULL, tileBytes);
                    g_SpriteCellCache[actor->spriteCellIndex].tileNumbers[frame] = tileNumber;
                }
                else
                {
                    tileNumber = g_SpriteCellCache[actor->spriteCellIndex].tileNumbers[frame];
                    upload = 0;
                }
                g_SpriteCellCache[actor->spriteCellIndex].refCounts[frame]++;
                if (frame != actor->previousAnimationFrame)
                {
                    g_SpriteCellCache[actor->spriteCellIndex]
                        .refCounts[actor->previousAnimationFrame]--;
                    if (g_SpriteCellCache[actor->spriteCellIndex]
                            .refCounts[actor->previousAnimationFrame] == 0)
                    {
                        sprite_freeObjTilesDeferred(actor->tileNumber, actor->gfxSize);
                        actor->tileNumber = 0xffff;
                        g_SpriteCellCache[actor->spriteCellIndex]
                            .tileNumbers[actor->previousAnimationFrame] = 0xffff;
                    }
                }
            }
        }
        else
        {
            tileNumber = sprite_allocObjTiles(NULL, tileBytes);
            if (actor->tileNumber != 0xffff)
            {
                sprite_freeObjTilesDeferred(actor->tileNumber, actor->gfxSize);
                actor->tileNumber = 0xffff;
            }
            if (actor->spriteFlags & 0x40)
            {
                for (i = 0; i < 4; i++)
                {
                    if ((s8)actor->sprite.slotPartIds[i] != -1)
                    {
                        u32 bytes = sprite_getSlotFrameInfo(actor, dimensions, i);
                        u32 slot = sprite_allocObjTiles(NULL, bytes);
                        if (actor->sprite.slotTileNumbers[i] != 0xffff)
                        {
                            sprite_freeObjTilesDeferred(actor->sprite.slotTileNumbers[i],
                                                        actor->sprite.slotGfxSizes[i]);
                            actor->sprite.slotTileNumbers[i] = 0xffff;
                        }
                        if (slot != 0xffff)
                        {
                            actor->sprite.slotTileNumbers[i] = slot;
                            actor->sprite.slotGfxSizes[i] = bytes;
                        }
                        else
                        {
                            actor->sprite.slotTileNumbers[i] |= 0xffff;
                            actor->sprite.slotGfxSizes[i] = bytes;
                        }
                    }
                }
            }
        }
        if (submit != TRUE)
        {
            tileNumber = 0xffff;
        }
        actor->tileNumber = tileNumber;
        actor->oam.tileNumber = tileNumber;
        actor->gfxSize = tileBytes;
        if (submit == TRUE && tileNumber != 0xffff && upload == 1)
        {
            u32 value = actor->spriteFlags;
            u8 *drawFlags;
            upload &= value;
            drawFlags = &actor->spriteFlags;
            if (upload)
            {
                tiles = actor_getFrameGfx(g_SpriteCellCache[actor->spriteCellIndex].owner);
                sprite_uploadActorFrame(g_SpriteCellCache[actor->spriteCellIndex].owner, tiles);
            }
            else
            {
                tiles = actor_getFrameGfx(actor);
                sprite_uploadActorFrame(actor, tiles);
            }
            if (*drawFlags & 0x40)
            {
                for (i = 0; i < 4; i++)
                {
                    if ((s8)actor->sprite.slotPartIds[i] != -1)
                    {
                        u8 partByte = (tiles = sprite_getSlotFrameGfx(actor, i),
                                       actor->sprite.slotPartIds[i]);
                        s32 part = (s8)partByte;
                        s32 pose = (s8)actor->sprite.pose;
                        u8 facingByte = actor->sprite.facingVariant;
                        s32 facing = (s8)facingByte;
                        const SpriteAnimation *const *bank =
                            (const SpriteAnimation *const *)actor->sprite.animationBank;
                        const SpriteFrameSetHeader *frames =
                            getSlotFrames(bank, part, pose, facing);
                        s32 frame;
                        u8 slotFlags = actor->sprite.slotFlags[i];
                        u32 loop = slotFlags & 4;
                        if (loop || usesPreviousFrame(slotFlags))
                        {
                            frame = actor->currentAnimationFrame - 1;
                            if (frame < 0)
                            {
                                if (loop)
                                {
                                    frame = frames->frameCount - 1;
                                }
                                else
                                {
                                    frame = 0;
                                }
                            }
                        }
                        else
                        {
                            frame = actor->currentAnimationFrame;
                        }
                        bank = (const SpriteAnimation *const *)actor->sprite.animationBank;
                        sprite_uploadFrame(getSlotFrames(bank, part, pose, facing), tiles,
                                           actor->sprite.slotTileNumbers[i], frame,
                                           actor->sprite.slotGfxSizes[i]);
                    }
                }
            }
        }
        actor->flags.p |= ACTOR_FLAG_ANIMATION_FRAME_CHANGED;
    }
done:
    actor->previousAnimationFrame = actor->currentAnimationFrame;
}
