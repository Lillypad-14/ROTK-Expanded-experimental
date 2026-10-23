#include "match_hacks.h"
#include "mission.h"
#include "sprite.h"
#include "variables.h"

static inline SpriteCellCacheEntry *cacheEntry(u32 offset)
{
    return (SpriteCellCacheEntry *)(offset + (u32)g_SpriteCellCache);
}

/**
 * Release an actor's graphics and return it to its owning pool.
 *
 * @return Zero on release, or -1 for an unused actor.
 *
 * @romaddress 0x080009e4
 */
s32 sprite_free(Actor *actor)
{
    if (actor->type != ACTOR_TYPE_STORAGE_MAX)
    {
        // HACK: preserve the original flags-address registers across cleanup calls.
        MATCH_PIN(u32 *, flagsRead, "r0") = &actor->flags.p;
        u32 actorFlags;
        MATCH_PIN(u32 *, flags, "r5");
        actorFlags = *flagsRead;
        flags = flagsRead;
        if ((s32)actorFlags < 0)
        {
            mission_script_setRecordActor(NULL, actor->recordGroup, actor->recordIndex);
        }
        sprite_releasePalette(actor);
        actor->type |= ACTOR_TYPE_STORAGE_MAX;
        {
            u8 spriteFlags = actor->oam.affineMode;
            if (spriteFlags == OAM_AFFINE_ON || spriteFlags == OAM_AFFINE_DOUBLE)
            {
                sprite_disableScale(actor);
            }
        }
        if (!(*flags & ACTOR_FLAG_FREEZE_GFX))
        {
            u32 gfxSize = actor->gfxSize;
            if (gfxSize != 0 && actor->tileNumber != 0xffff)
            {
                // HACK: preserve the original flags and tile-address allocation.
                MATCH_PIN(u32, spriteFlags, "r3") = actor->spriteFlags;

                if (spriteFlags & 3)
                {
                    if (g_SpriteFrameSubmitted == 1)
                    {
                        u32 frame;
                        u32 cell = actor->spriteCellIndex;
                        u32 refCount;
                        u32 cacheBase;

                        if (spriteFlags & 1)
                        {
                            frame = 0;
                        }
                        else
                        {
                            frame = actor->previousAnimationFrame;
                        }
                        cacheBase = (u32)g_SpriteCellCache;
                        cell *= sizeof(SpriteCellCacheEntry);
                        refCount = ((SpriteCellCacheEntry *)(cell + cacheBase))->refCounts[frame];
                        if (refCount != 0)
                        {
                            u16 tiles;

                            refCount = --cacheEntry(cell)->refCounts[frame];
                            tiles = cacheEntry(cell)->tileNumbers[frame];
                            if (refCount == 0 && tiles != 0xffff)
                            {
                                sprite_freeObjTiles(tiles, actor->gfxSize);
                                actor->tileNumber |= 0xffff;
                                cacheEntry(cell)->tileNumbers[frame] |= 0xffff;
                            }
                        }
                    }
                }
                else if (g_SpriteFrameSubmitted == 1)
                {
                    sprite_freeObjTiles(actor->tileNumber, gfxSize);
                    if (actor->spriteFlags & 0x40)
                    {
                        u32 i;

                        for (i = 0; i < 4; i++)
                        {
                            if (actor->sprite.slotTileNumbers[i] != 0xffff)
                            {
                                sprite_freeObjTiles(actor->sprite.slotTileNumbers[i],
                                                    actor->sprite.slotGfxSizes[i]);
                                actor->sprite.slotTileNumbers[i] |= 0xffff;
                                actor->sprite.slotGfxSizes[i] = 0;
                            }
                        }
                    }
                }
            }
        }
        if (actor->listId == ACTOR_LIST_MAIN)
        {
            if (g_ActorListMain.tail == actor)
            {
                g_ActorListMain.tail = actor->previous;
            }
            pool_free(&g_ActorPoolMainFreeList, &g_ActorListMain, actor);
        }
        else
        {
            if (g_ActorListOverlay.tail == actor)
            {
                g_ActorListOverlay.tail = actor->previous;
            }
            pool_free(&g_ActorPoolOverlayFreeList, &g_ActorListOverlay, actor);
        }
        return 0;
    }
    return -1;
}
