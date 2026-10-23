#include "game.h"
#include "sprite.h"
#include "variables.h"

/**
 * Update one actor and submit its collision and rendering work for the frame.
 *
 * @romaddress 0x08002050
 */
void actor_tick(Actor *actor, bool commit)
{
    u32 *flags = &actor->flags.p;
    u32 currentFlags;
    u8 spriteFlags;

    if (*flags & ACTOR_FLAG_PENDING_REMOVE)
    {
        sprite_free(actor);
        return;
    }
    *flags &= ~(ACTOR_FLAG_ON_SCREEN | ACTOR_FLAG_ON_ACTIVE_PLAYER_SCREEN);
    actor_updateVisibility(actor);
    currentFlags = *flags;
    spriteFlags = actor->spriteFlags;
    if (!(g_GameFlags.p & GAME_FLAG_11) &&
        (currentFlags & (ACTOR_FLAG_ON_SCREEN | ACTOR_FLAG_TICK_ALWAYS)))
    {
        if (actor->tickCallback != NULL && !(currentFlags & ACTOR_FLAG_TICK_SUPPRESSED))
        {
            actor->tickCallback(actor);
            currentFlags = *flags;
        }
        if (currentFlags & ACTOR_FLAG_TICK_ONLY)
        {
            return;
        }
        if (!(currentFlags & (ACTOR_FLAG_TICK_SUPPRESSED | ACTOR_FLAG_ALWAYS_VISIBLE)))
        {
            actor_tickAnimation(actor);
            if (actor->sprite.scaleFramesLeft != 0)
            {
                sprite_tickScale(actor);
            }
            actor_integrateMovement(actor);
            if (currentFlags & ACTOR_FLAG_TILE_COLLISION)
            {
                actor_resolveTileCollision(actor);
            }
            if ((currentFlags & (ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_PENDING_REMOVE)) ==
                ACTOR_FLAG_COLLISION_CHECK)
            {
                g_CollisionCheckQueue[g_CollisionCheckQueueCount++] = actor;
            }
        }
        else if ((currentFlags & (ACTOR_FLAG_ALWAYS_VISIBLE | ACTOR_FLAG_RENDER)) ==
                 (ACTOR_FLAG_ALWAYS_VISIBLE | ACTOR_FLAG_RENDER))
        {
            actor_tickAnimation(actor);
            g_SpriteFixedRenderQueue[g_SpriteFixedRenderQueueCount++] = actor;
        }
        if ((g_GameFlags.p & GAME_FLAG_WORLD_PARKED) && commit == TRUE)
        {
            actor->coords = actor->renderCoords;
            actor_applyRenderOscillation(actor);
        }
    }
    if ((currentFlags & (ACTOR_FLAG_ALWAYS_VISIBLE | ACTOR_FLAG_ON_SCREEN | ACTOR_FLAG_RENDER)) ==
        (ACTOR_FLAG_ON_SCREEN | ACTOR_FLAG_RENDER))
    {
        if (spriteFlags & 1)
        {
            if (g_SpriteCellCache[actor->spriteCellIndex].refCounts[0] != 0)
            {
                g_SpriteCellCache[actor->spriteCellIndex].refCounts[0] = 0;
                if (g_SpriteCellCache[actor->spriteCellIndex].tileNumbers[0] != 0xffff)
                {
                    if (commit == TRUE)
                    {
                        sprite_freeObjTilesDeferred(
                            g_SpriteCellCache[actor->spriteCellIndex].tileNumbers[0],
                            g_SpriteCellCache[actor->spriteCellIndex].gfxSize);
                    }
                    g_SpriteCellCache[actor->spriteCellIndex].tileNumbers[0] |= 0xffff;
                }
            }
            actor->tileNumber = 0xffff;
        }
        g_SpriteWorldRenderQueue[g_SpriteWorldRenderQueueCount++] = actor;
    }
    if (commit == TRUE &&
        !(currentFlags & (ACTOR_FLAG_ON_ACTIVE_PLAYER_SCREEN | ACTOR_FLAG_FREEZE_GFX)) &&
        actor->tileNumber != 0xffff && !(actor->spriteFlags & 1))
    {
        if (spriteFlags & 2)
        {
            u32 refs = --g_SpriteCellCache[actor->spriteCellIndex]
                             .refCounts[actor->previousAnimationFrame];

            if (refs == 0)
            {
                sprite_freeObjTilesDeferred(actor->tileNumber, actor->gfxSize);
            }
            actor->tileNumber |= 0xffff;
        }
        else
        {
            sprite_freeObjTilesDeferred(actor->tileNumber, actor->gfxSize);
            actor->tileNumber = 0xffff;
            if (actor->spriteFlags & 0x40)
            {
                u32 i;

                for (i = 0; i < 4; i++)
                {
                    sprite_freeObjTilesDeferred(actor->sprite.slotTileNumbers[i],
                                                actor->sprite.slotGfxSizes[i]);
                    actor->sprite.slotTileNumbers[i] = 0xffff;
                    actor->sprite.slotGfxSizes[i] = 0;
                }
            }
        }
    }
}
