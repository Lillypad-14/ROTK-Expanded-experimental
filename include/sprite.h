#pragma once

#include "actor.h"
#include "spriteAnimation.h"
#include "types.h"

typedef struct OamMatrixParams
{
    s16 scaleX;
    s16 scaleY;
    Angle16 angle;
    u16 padding;
} OamMatrixParams;

#define SPRITE_CELL_CACHE_SIZE 0x294
#define SPRITE_PALETTE_RELEASE_DELAY 90

typedef struct SpriteCellCacheEntry
{
    Actor *owner;
    u16 gfxSize;
    u16 tileNumbers[12];
    u8 refCounts[12];
} SpriteCellCacheEntry;

void sprite_setAnimation(Actor *actor, const SpriteAnimation *animation);
void sprite_bindAnimation(Actor *actor, const SpriteAnimation *animation);

/**
 * Select an animation frame directly: resets the frame countdown, copies the descriptor
 * header bytes, and refreshes the trigger boxes from the frame data.
 */
void sprite_setAnimationFrame(Actor *actor, u8 frameIndex);

void sprite_getAnimationFrameSize(const SpriteAnimation *animation, u32 *sizeOut, u8 frameIndex);

Actor *actor_allocMain(void);
Actor *actor_allocOverlay(void);
void sprite_setPosition(Actor *actor, u32 x, u32 y);

/**
 * Walk an actor list oldest-first (from list->tail), actor_tick'ing each object.
 *
 * @param commit true also commits the frame (gfx upload + OAM compose)
 */
u8 actor_tickAllInList(ActorList *list, bool commit);
void sprite_flushList(ActorList *list);
s32 sprite_pushOam(u8 index, OamEntryWords *entry);
u8 actor_tickAll(void);
void sprite_endFrame(void);

/** Look up the pixel width and height of an OBJ shape/size pair. */
void sprite_getOamShapeDims(u32 shape, u32 size, Vector2Int *out);

/** Hide an entry of the second OAM shadow page. */
void sprite_hideOamEntry(u32 index);
/** Hide every OAM shadow entry, DMA the shadow to OAM, and reset the alloc count. */
void sprite_resetOam(void);

/*
 * OBJ VRAM tile allocator: g_ObjTileBitmap has one bit per 32-byte tile (1024 tiles,
 * 32 KiB). g_ObjTileWorldHighWater counts the tiles handed to actors while the world is not
 * parked and is never lowered by a free; while GAME_FLAG_WORLD_PARKED is set, allocations
 * search above it so overlay sprites leave the parked world's tiles alone.
 */

void sprite_resetTileAllocator(void);
void sprite_resetTileAllocator2(void);
void sprite_resetTileAllocatorForMode(u8 displayMode);

/**
 * Allocate OBJ VRAM tiles for gfxSize bytes of character data and, with an actor, upload
 * its frame there. The first-fit bit search runs from IWRAM (0x03007690).
 *
 * @return the first tile number, 0xffff when no run is free
 */
u32 sprite_allocObjTiles(Actor *actor, u32 gfxSize);

void sprite_freeObjTiles(u16 tileNumber, u32 gfxSize);
void sprite_clearObjTileBits(u16 tileNumber, u16 tileCount);

void sprite_freeObjTilesDeferred(u16 tileNumber, u32 gfxSize);

void sprite_applyDeferredObjTileFrees(void);

void sprite_selectAnimation(Actor *actor, void *animationBank, s8 pose, s8 facingVariant);

void sprite_initSlots(Actor *actor);
void sprite_attachSlot(Actor *actor, u8 slot, u8 partId, u8 paletteNumber);
void sprite_detachSlot(Actor *actor, u8 slot);
void sprite_setSlotDrawOrder(Actor *actor, u8 slot, u8 drawOrder);
// Set/clear per-slot flag bits (see SpriteRender.slotFlags); only partly decoded (bits
// 2/3 gate the slot's frame advance).
void sprite_setSlotFlag0(Actor *actor, u8 slot);
void sprite_clearSlotFlag0(Actor *actor, u8 slot);
void sprite_setSlotFlag1(Actor *actor, u8 slot);
void sprite_clearSlotFlag1(Actor *actor, u8 slot);
void sprite_setSlotFlag2(Actor *actor, u8 slot);
void sprite_clearSlotFlag2(Actor *actor, u8 slot);
void sprite_getOamPosition(Actor *actor, Vector2Int *out);

/**
 * Slot equivalent of actor_getAnimationFrame: the slot's current-frame packed gfx id;
 * two frame bytes go to out.
 */
u16 sprite_getSlotFrameInfo(Actor *actor, u32 *out, u8 slot);
/** The slot's current-frame tile data pointer. */
void *sprite_getSlotFrameGfx(Actor *actor, u8 slot);

void actor_tick(Actor *actor, bool commit);
void actor_tickAnimation(Actor *actor);
void actor_integrateMovement(Actor *actor);
void actor_updateVisibility(Actor *actor);
void actor_applyRenderOscillation(Actor *actor);
void actor_updateRenderFollow(Actor *actor);
void actor_resolveTileCollision(Actor *actor);
void actor_unknown08033e64(Actor *actor);
s32 sprite_free(Actor *actor);
/**
 * Compose the sprite's OAM entries: screen position, the multi-sprite slots in draw
 * order (7..4 before the main sprite, 3..0 after), then the main sprite. Returns 0 if not
 * submitted; 2 also queues the sprite for post-commit processing.
 */
u8 sprite_composeOam(Actor *actor);
/**
 * Commit a submitted sprite's graphics: allocate OAM slots and upload the current
 * frame's tiles for the main sprite and each attached slot, releasing stale slots.
 */
void sprite_commitGfx(Actor *actor, bool commit);

void sprite_setScale(Actor *actor, s16 scaleX, s16 scaleY, Angle16 angle);
void sprite_startScale(Actor *actor, s32 startX, s32 startY, s32 targetX, s32 targetY, s32 frames);
void sprite_tickScale(Actor *actor);
void sprite_disableScale(Actor *actor);
u16 sprite_ensureOamMatrix(Actor *actor);
void sprite_setObjTileBitsDeferred(u16 tileNumber, u16 tileCount);
void sprite_setOamMatrixIndex(OamEntry *oam, u16 index);
u16 sprite_getOamMatrixIndex(OamEntry *oam);
u16 sprite_allocOamMatrix(void);
void sprite_freeOamMatrix(u16 index);
void sprite_buildOamMatrices(void);
void sprite_releasePalette(Actor *actor);
