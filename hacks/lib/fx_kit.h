#pragma once

// Skill visuals built from the stock FX actor system: the 23 FX descriptor rows, the OBJ palette
// slots, recolored custom palettes and enemy animations played as effects.

#include "actor.h"
#include "types.h"
#include "vector2.h"

/** Stock FX descriptor rows (fx_spawnOnActor fxType), named by their look. */
enum HackLibFxShape
{
    FX_SHAPE_STAR_BLOOM = 0,     // star that opens into a dotted ring
    FX_SHAPE_SPARKLE_RING = 2,   // ring of sparkles
    FX_SHAPE_DIAMOND_RING = 3,   // diamond star that settles into a ring with a center
    FX_SHAPE_RISING_STREAKS = 4, // streaks rising and fanning out
    FX_SHAPE_POOL_FLASH = 5,     // small ground pool, then a star flash
    FX_SHAPE_BUBBLE_RINGS = 6,   // bubbling rings, multicolor
    FX_SHAPE_GROUND_POOL = 7,    // ground pool that spreads, then an outline ring
    FX_SHAPE_TENDRIL_BURST = 8,  // tendril rising and bursting into specks
    FX_SHAPE_CHECKER_RING = 9,   // checkered ring
    FX_SHAPE_SMOKE_RISE = 10,    // big cloud rising and scattering
    FX_SHAPE_SMOKE_SMALL = 11,   // small cloud rising and scattering
    FX_SHAPE_RING_SPARKLES = 12, // ring, then sparkles rising, multicolor
    FX_SHAPE_SKY_RAIN = 13,      // screen-sized fall of tiny streaks
    FX_SHAPE_STREAK_RAIN = 14,   // streaks falling
    FX_SHAPE_LIGHTNING = 15,     // tall vertical bolt
    FX_SHAPE_SPARK = 16,         // small four-point spark
    FX_SHAPE_OVAL_RING = 17,     // dotted oval ring expanding
    FX_SHAPE_TENDRIL_STAR = 18,  // bright core with lightning tendrils
    FX_SHAPE_WIDE_RING = 19,     // checkered ring expanding wide
    FX_SHAPE_SHOCKWAVE = 20,     // complete ground wave, multicolor
    FX_SHAPE_SHOCKWAVE_ALT = 21, // complete ground wave, alternate stock row
    FX_SHAPE_DEBRIS = 22,        // debris puffs, follows the parent's animation frame
    FX_SHAPE_COUNT = 23,
};

/**
 * Effect color. Values below 16 are OBJ palette slots. Slots 2..8 hold ObjPalettes[2..8] in
 * every level. Direct slots 9..15 must already be occupied and gain a shared reference.
 * FX_COLOR_OBJ_PALETTE and FX_COLOR_CUSTOM borrow a free enemy slot (9..15).
 */
typedef u16 HackLibFxColor;

enum HackLibFxFixedColor
{
    FX_COLOR_WHITE = 2,
    FX_COLOR_EMBER = 3,
    FX_COLOR_GREEN = 4,
    FX_COLOR_BLUE = 5,
    FX_COLOR_NAZGUL = 6, // the Nazgul enemy and Ringwraith hero palette
    FX_COLOR_NIGHT = 7,  // violet, the stock Nightburst color
    FX_COLOR_GOLD = 8,   // the stock Sunburst color
};

/** Recolored palettes of fx_kit.c (FX_COLOR_CUSTOM index). */
enum HackLibFxCustomPalette
{
    FX_PALETTE_SHADOW = 0, // near-black violet
    FX_PALETTE_MORGUL = 1, // sickly pale green
    FX_PALETTE_BLOOD = 2,  // deep red
    FX_PALETTE_GHOST = 3,  // cold white-cyan
    FX_PALETTE_VENOM = 4,  // bright lime
    FX_PALETTE_COUNT = 5,
};

#define FX_COLOR_OBJ_PALETTE(objPaletteId) ((HackLibFxColor)(0x100 | (objPaletteId)))
#define FX_COLOR_CUSTOM(customPalette) ((HackLibFxColor)(0x200 | (customPalette)))
#define FX_COLOR_SHADOW FX_COLOR_CUSTOM(FX_PALETTE_SHADOW)
#define FX_COLOR_MORGUL FX_COLOR_CUSTOM(FX_PALETTE_MORGUL)
#define FX_COLOR_BLOOD FX_COLOR_CUSTOM(FX_PALETTE_BLOOD)
#define FX_COLOR_GHOST FX_COLOR_CUSTOM(FX_PALETTE_GHOST)
#define FX_COLOR_VENOM FX_COLOR_CUSTOM(FX_PALETTE_VENOM)

/**
 * Rows of the native timed FX table (fx_spawnTimedOnActor). Most follow the slot hue. SHIELD
 * is multicolor: slot 6 (FX_COLOR_NAZGUL) is the stock blue bubble.
 */
enum HackLibFxAura
{
    FX_AURA_BIG_RING = 2,      // upright ring that thickens
    FX_AURA_RISING_SPARKS = 3, // sparks streaming upward, 26 frames
    FX_AURA_FLAME_CROWN = 4,   // spiky ground ring that grows, the stock triple-damage aura
    FX_AURA_SWIRL = 7,         // arcs circling the feet
    FX_AURA_SHIELD = 9,        // bubble forming around the actor, the stock Gandalf shield
    FX_AURA_GROUND_RINGS = 10, // concentric ground ellipses
    FX_AURA_ORBIT = 12,        // small arc orbiting the feet
    FX_AURA_COUNT = 14,
};

/** Where an effect draws relative to the actor it follows. */
enum HackLibFxLayer
{
    FX_LAYER_BEHIND = 0,
    FX_LAYER_FRONT = 1,
};

/** One-shot effect on an actor. It follows the actor and despawns when its animation ends. */
Actor *hackLib_fxBurst(Actor *on, u8 shape, HackLibFxColor color, u8 layer);

/** Loop the selected stock burst shape for 1..32767 ticks. */
Actor *hackLib_fxTimed(Actor *on, u8 shape, HackLibFxColor color, s32 frames, u8 layer);

/** One-shot effect left at a world position. origin only seeds the draw layer. */
Actor *hackLib_fxAt(Actor *origin, const Vector2Fp16 *pos, u8 shape, HackLibFxColor color);

/**
 * Pixels the upright hoop shapes (CHECKER_RING, WIDE_RING) are raised so they ring the body
 * instead of sinking half into the floor. fxDetach applies it, and fxBurst detaches hoops.
 */
#define FX_HOOP_LIFT 16

/** A native timed FX row (HackLibFxAura) on an actor for 1..32767 ticks. */
Actor *hackLib_fxAura(Actor *on, u8 aura, HackLibFxColor color, s32 frames, u8 layer);

/**
 * A sprite animation played once as an effect on an actor, frameTicks ticks per frame. Frames
 * must fit 2304 bytes of tiles. Custom sprites live in fx_sprites.h. Returns NULL when no
 * palette slot or actor is free.
 */
Actor *hackLib_fxSprite(Actor *on, const SpriteAnimation *animation, HackLibFxColor color,
                        u8 frameTicks, u8 layer);

// Imported sprite palettes use stable IDs 0..31 in a separate shared-slot tag range.
Actor *hackLib_fxSpritePalette(Actor *on, const SpriteAnimation *animation, const u16 *palette,
                               u8 paletteId, u8 frameTicks, u8 layer);

/**
 * An enemy's pose animation (facing 0) played once as an effect on an actor, in the enemy's
 * own palette, frameTicks ticks per frame. Returns NULL when no palette slot or actor is free.
 */
Actor *hackLib_fxEnemyAnim(Actor *on, u8 enemyType, u8 pose, u8 frameTicks, u8 layer);

/**
 * Step fx every frameTicks ticks. Stock shapes step every 1 or 2 ticks. Returns fx, which may
 * be NULL.
 */
Actor *hackLib_fxPace(Actor *fx, u8 frameTicks);

/**
 * Load OBJ palette slot with base, colors first..first+count-1 mixed toward color by
 * strength / FX_TINT_FULL. Strength 0 restores base. Only the player slots below
 * FX_TINT_SLOT_COUNT tint. Use it to make part of a hero sprite glow, such as a blade.
 */
#define FX_TINT_FULL 16
#define FX_TINT_SLOT_COUNT 2
void hackLib_fxTintSlot(u8 slot, const u16 *base, u8 first, u8 count, u16 color, u8 strength);

/** Stop following the parent and stay at pos. */
void hackLib_fxDetach(Actor *fx, const Vector2Fp16 *pos);

/**
 * Freeze fx on its last animation frame for frames, then despawn it. Holds up to
 * FX_HOLD_COUNT effects at once. Returns FALSE when all holds are busy.
 */
#define FX_HOLD_COUNT 6
bool32 hackLib_fxHold(Actor *fx, u16 frames);

/** Despawn an effect now. */
void hackLib_fxRemove(Actor *fx);
