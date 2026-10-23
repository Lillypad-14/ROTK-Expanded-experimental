#include "fx.h"
#include "game.h"
#include "sprite.h"
#include "variables.h"

union OamFlipView {
    u8 p;
    struct
    {
        u8 low : 4;
        u8 horizontal : 1;
        u8 vertical : 1;
        u8 size : 2;
    } __attribute__((packed)) d;
} __attribute__((packed));

/**
 * Tick an actor list, resolve collisions and submit sprites and particles in draw order.
 *
 * @return Nonzero when an actor submits visible rendering work.
 *
 * @romaddress 0x08000708
 */
u32 actor_tickAllInList(ActorList *list, bool commit)
{
    void (*sort)(void) = (void (*)(void))g_ActorRenderSortCode;
    void (*collide)(void) = (void (*)(void))g_ActorCollisionCode;
    u32 result = 0;
    u32 layer;
    Actor *actor;
    u32 index;

    g_SpriteFixedRenderQueueCount = 0;
    g_SpriteWorldRenderQueueCount = 0;
    g_SpritePostCommitQueueCount = 0;
    g_CollisionCheckQueueCount = 0;
    layer = 0;
    if ((g_GameFlags.p & (GAME_FLAG_18 | GAME_FLAG_WORLD_PARKED)) == GAME_FLAG_WORLD_PARKED)
    {
        actor = list->head;
        while (actor != NULL)
        {
            Actor *next = actor->next;
            actor_tick(actor, commit);
            actor = next;
        }
    }
    else
    {
        g_ActorTickCursor = list->tail;
        actor = g_ActorTickCursor;
        while (actor != NULL)
        {
            Actor *previous = actor->previous;
            actor_tick(actor, commit);
            actor = previous;
        }
        if (!(g_GameFlags.p & GAME_FLAG_11) && (u32)g_CollisionCheckQueueCount > 1 &&
            list == &g_ActorListMain)
        {
            collide();
        }
        for (actor = list->head; actor != NULL; actor = actor->next)
        {
            if (actor->renderFollowTarget != NULL)
            {
                actor_updateRenderFollow(actor);
            }
            actor->coords = actor->renderCoords;
            actor_applyRenderOscillation(actor);
        }
    }
    if ((u32)g_SpriteWorldRenderQueueCount > 1)
    {
        sort();
    }
    index = g_SpriteFixedRenderQueueCount;
    while (index != 0)
    {
        u32 nextIndex;
        u32 flags;
        actor = g_SpriteFixedRenderQueue[index - 1];
        flags = actor->flags.p & ACTOR_FLAG_FREEZE_GFX;
        nextIndex = index - 1;
        if (!flags)
        {
            sprite_commitGfx(actor, commit);
        }
        if (commit == TRUE)
        {
            index = sprite_composeOam(actor);
            if (index != 0)
            {
                result = 1;
                if (index == 2)
                {
                    g_SpritePostCommitQueue[g_SpritePostCommitQueueCount++] = actor;
                }
            }
        }
        index = nextIndex;
    }
    fx_buildRenderQueues();
    for (index = 0; index < (u32)g_SpriteWorldRenderQueueCount;)
    {
        u32 nextIndex;
        actor = g_SpriteWorldRenderQueue[index];
        if (!(actor->flags.p & ACTOR_FLAG_FREEZE_GFX))
        {
            sprite_commitGfx(actor, commit);
        }
        nextIndex = index + 1;
        if (commit == TRUE)
        {
            if (actor->oam.bgPriority != layer && layer < actor->oam.bgPriority)
            {
                do
                {
                    fx_renderLayer(layer);
                    layer++;
                } while (layer < actor->oam.bgPriority);
            }
            index = sprite_composeOam(actor);
            if (actor->spriteFlags & 0x10)
            {
                ((union OamFlipView *)((u8 *)&actor->oam + 3))->d.horizontal = 1;
                sprite_composeOam(actor);
                ((union OamFlipView *)((u8 *)&actor->oam + 3))->d.horizontal = 0;
            }
            if (index != 0)
            {
                result = 1;
                if (index == 2)
                {
                    g_SpritePostCommitQueue[g_SpritePostCommitQueueCount++] = actor;
                }
            }
        }
        index = nextIndex;
    }
    if (commit == TRUE)
    {
        while (layer < 4)
        {
            fx_renderLayer(layer);
            layer++;
        }
    }
    if (g_GameFlags.p & 0x30000)
    {
        for (index = 0; index < (u32)g_SpriteWorldRenderQueueCount; index++)
        {
            actor = g_SpriteWorldRenderQueue[index];
            if ((actor->flags.p & ACTOR_FLAG_HIT_FLASH) && commit == TRUE)
            {
                actor->spriteFlags |= 0x80;
                sprite_composeOam(actor);
                actor->spriteFlags &= ~0x80;
            }
        }
    }
    if (commit == TRUE)
    {
        sprite_buildOamMatrices();
    }
    return result;
}
