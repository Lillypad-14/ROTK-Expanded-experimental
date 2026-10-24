#include "fx_kit.h"
#include "fx_shockwave.h"
#include "gba_io.h"
#include "lib/actor_ref.h"
#include "lib/profile.h"

#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "memory.h"
#include "mission.h"
#include "sprite.h"
#include "variables.h"

// actor_applyTypeDefaults' OBJ palette slot table: the ObjPalettes id loaded in each slot.
#define PALETTE_SLOT_TAGS ((u8 *)0x030053d0)
// Actors using each slot. Freeing an actor with ACTOR_FLAG_DEFAULTS_APPLIED decrements it.
#define PALETTE_SLOT_REFS ((u8 *)0x030053e0)
#define PALETTE_SLOT_FREE 0x5a
// Slots 2..8 hold ObjPalettes[2..8] in every level.
#define FIRST_FIXED_SLOT 2
#define FIRST_SHARED_SLOT 9
#define SLOT_COUNT 16
#define NO_SLOT 0xff
// Slot tags of the custom palettes, above every ObjPalettes id.
#define CUSTOM_TAG_BASE 0x80
#define IMPORTED_TAG_BASE 0x90
_Static_assert(CUSTOM_TAG_BASE + FX_PALETTE_COUNT <= IMPORTED_TAG_BASE, "FX palette tag overlap");
_Static_assert(ARRAY_COUNT(ObjPalettes) <= CUSTOM_TAG_BASE, "native palette tag overlap");

#define ENEMY_PALETTE_IDS ((const u8 *)0x0806d27c)

// Actor.spriteFlags bits that offset a render-following effect behind or in front.
#define SPRITE_FLAGS_FOLLOW_LAYER 0x0c
// Actor.as words fx_spawnOnActor seeds with the spawn position.
#define FX_SPAWN_POS_OFFSET 0x20

// Palette 7 recolored by hacks/tools/fx_palette.py.
static const u16 CustomPalettes[FX_PALETTE_COUNT][16] = {
    [FX_PALETTE_SHADOW] = {0x7c1f, 0x39ce, 0x398d, 0x356c, 0x312b, 0x2ce9, 0x28a8, 0x2887, 0x390b,
                           0x38eb, 0x34ea, 0x30c9, 0x28c8, 0x24c7, 0x1485, 0x0c42},
    [FX_PALETTE_MORGUL] = {0x7c1f, 0x7fff, 0x73fd, 0x6bfb, 0x5bf8, 0x47b3, 0x334f, 0x232b, 0x3ff2,
                           0x3bf2, 0x3ff2, 0x3bf1, 0x332f, 0x3acf, 0x25ca, 0x10e4},
    [FX_PALETTE_BLOOD] = {0x7c1f, 0x739c, 0x5efb, 0x4a59, 0x3598, 0x1cd6, 0x0835, 0x0414, 0x147c,
                          0x105c, 0x105a, 0x0c58, 0x1074, 0x1cd1, 0x148b, 0x0425},
    [FX_PALETTE_GHOST] = {0x7c1f, 0x7fff, 0x7ffe, 0x7ffd, 0x7ffc, 0x7fda, 0x7fd9, 0x7fd7, 0x7fd9,
                          0x7fd9, 0x7fd9, 0x7fd9, 0x7fd9, 0x6f57, 0x462f, 0x2107},
    [FX_PALETTE_VENOM] = {0x7c1f, 0x7fff, 0x73fe, 0x67fc, 0x57fb, 0x43f8, 0x33f6, 0x1bf4, 0x37f7,
                          0x33f7, 0x33f7, 0x33f7, 0x37f7, 0x3f76, 0x2a2e, 0x1106},
};

typedef struct FxHold
{
    // held effect, NULL for a free hold
    Actor *fx;
    u32 generation;

    // frames left before the effect despawns
    u16 framesLeft;
} FxHold;

static FxHold holds[FX_HOLD_COUNT];

// Tinted colors per tintable slot, kept until the queued palette write commits.
static u16 tintedSlots[FX_TINT_SLOT_COUNT][16];

static void loadSlot(u8 slot, const u16 *colors)
{
    if (g_GameFlags.p & GAME_FLAG_OVERLAY)
    {
        memory_memcpy(g_ObjPaletteSnapshot + slot * 16, colors, 32);
    }
    else
    {
        gfx_queuePaletteWrite(colors, slot << 4, 16);
    }
}

/** A shared slot holding colors under tag, loading them into a free slot if needed. */
static u8 claimSlot(u8 tag, const u16 *colors)
{
    u8 free = NO_SLOT;
    u8 slot;

    for (slot = FIRST_SHARED_SLOT; slot < SLOT_COUNT; slot++)
    {
        if (PALETTE_SLOT_TAGS[slot] == tag)
        {
            if (PALETTE_SLOT_REFS[slot] == 0xff)
            {
                return NO_SLOT;
            }
            PALETTE_SLOT_REFS[slot]++;
            return slot;
        }
        if (free == NO_SLOT && PALETTE_SLOT_TAGS[slot] == PALETTE_SLOT_FREE)
        {
            free = slot;
        }
    }
    if (free != NO_SLOT)
    {
        PALETTE_SLOT_TAGS[free] = tag;
        PALETTE_SLOT_REFS[free]++;
        loadSlot(free, colors);
    }
    return free;
}

static void releaseSlot(u8 slot)
{
    if (slot >= FIRST_SHARED_SLOT && slot < SLOT_COUNT && PALETTE_SLOT_REFS[slot] != 0 &&
        --PALETTE_SLOT_REFS[slot] == 0)
    {
        PALETTE_SLOT_TAGS[slot] = PALETTE_SLOT_FREE;
    }
}

/** The slot for color, claimed for one actor. NO_SLOT when no shared slot is free. */
static u8 resolveColor(HackLibFxColor color)
{
    u8 index = color & 0xff;

    if ((color & 0xff00) == 0x100)
    {
        if (index >= ARRAY_COUNT(ObjPalettes))
        {
            return NO_SLOT;
        }
        return index >= FIRST_FIXED_SLOT && index < FIRST_SHARED_SLOT
                   ? index
                   : claimSlot(index, ObjPalettes[index]);
    }
    if ((color & 0xff00) == 0x200)
    {
        return index < FX_PALETTE_COUNT ? claimSlot(CUSTOM_TAG_BASE + index, CustomPalettes[index])
                                        : NO_SLOT;
    }
    if ((color & 0xff00) != 0 || index >= SLOT_COUNT)
    {
        return NO_SLOT;
    }
    if (index >= FIRST_SHARED_SLOT)
    {
        if (PALETTE_SLOT_TAGS[index] == PALETTE_SLOT_FREE || PALETTE_SLOT_REFS[index] == 0xff)
        {
            return NO_SLOT;
        }
        PALETTE_SLOT_REFS[index]++;
    }
    return index;
}

u8 hackLib_fxCap;

/** TRUE when hackLib_fxCap effects are already alive. */
static bool32 atFxCap(void)
{
    Actor *actor;
    u32 live = 0;

    if (hackLib_fxCap == 0)
    {
        return FALSE;
    }
    for (actor = g_ActorListMain.head; actor != NULL; actor = actor->next)
    {
        if (actor->actionState == ACTOR_STATE_FX_ATTACHED && ++live >= hackLib_fxCap)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static bool validRequest(Actor *on, u8 shape, u8 layer)
{
    return on != NULL && shape < FX_SHAPE_COUNT && layer <= FX_LAYER_FRONT && !atFxCap();
}

static bool validAnimation(const SpriteAnimation *animation, u8 frameTicks)
{
    return animation != NULL && animation->frames != NULL &&
           animation->frames->maxFrameTileBytes <= 2304 && frameTicks != 0;
}

/** A burst of an upright hoop shape, drawn FX_HOOP_LIFT above its position. */
static bool32 isHoop(Actor *fx)
{
    u8 shape = fx->as.combat.appliedPose;

    return fx->type == FX_BURST_UNK_PARAM &&
           (shape == FX_SHAPE_CHECKER_RING || shape == FX_SHAPE_WIDE_RING);
}

/** Tie a claimed shared slot to fx so freeing fx releases it. */
static Actor *bindSlot(Actor *fx, u8 slot)
{
    if (fx == NULL)
    {
        releaseSlot(slot);
    }
    else if (slot >= FIRST_SHARED_SLOT)
    {
        fx->flags.p |= ACTOR_FLAG_DEFAULTS_APPLIED;
    }
    return fx;
}

Actor *hackLib_fxBurst(Actor *on, u8 shape, HackLibFxColor color, u8 layer)
{
    HackLibProfileSpan profile = hackLib_profileBegin(HACKLIB_PROFILE_FX);
    u8 slot;
    Actor *fx;

    if (!validRequest(on, shape, layer))
    {
        hackLib_profileEnd(profile);
        return NULL;
    }
    slot = resolveColor(color);
    if (slot == NO_SLOT)
    {
        hackLib_profileEnd(profile);
        return NULL;
    }
    fx = bindSlot(fx_spawnOnActor(on, shape, slot, layer), slot);
    if (fx != NULL)
    {
        fx->type = FX_BURST_UNK_PARAM;
        fx->tickCallback = (ActorCallback)(void *)fx_tickAttachment;
        // Stock shockwave rows contain opposite halves of the same ground wave.
        if (shape == FX_SHAPE_SHOCKWAVE || shape == FX_SHAPE_SHOCKWAVE_ALT)
        {
            sprite_setAnimation(fx, &HackLibFxGroundShockwave);
        }
        if (isHoop(fx))
        {
            hackLib_fxDetach(fx, &on->coords);
        }
    }
    hackLib_profileWork(HACKLIB_PROFILE_FX, fx != NULL);
    hackLib_profileEnd(profile);
    return fx;
}

static void tickTimedShape(Actor *fx)
{
    if (--fx->as.combat.venomDuration <= 0)
    {
        hackLib_fxRemove(fx);
        return;
    }
    if (fx->flags.p & ACTOR_FLAG_ANIMATION_END)
    {
        fx->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
        sprite_setAnimationFrame(fx, 0);
    }
    if (fx_tickAttachment(fx))
    {
        hackLib_fxRemove(fx);
    }
}

Actor *hackLib_fxTimed(Actor *on, u8 shape, HackLibFxColor color, s32 frames, u8 layer)
{
    if (frames <= 0 || frames > 32767)
    {
        return NULL;
    }
    Actor *fx = hackLib_fxBurst(on, shape, color, layer);
    if (fx != NULL)
    {
        fx->as.combat.venomDuration = frames;
        fx->tickCallback = tickTimedShape;
    }
    return fx;
}

Actor *hackLib_fxAura(Actor *on, u8 aura, HackLibFxColor color, s32 frames, u8 layer)
{
    u8 slot;
    Actor *fx;

    if (on == NULL || aura >= FX_AURA_COUNT || layer > FX_LAYER_FRONT || frames <= 0 ||
        frames > 32767 || atFxCap())
    {
        return NULL;
    }
    slot = resolveColor(color);
    if (slot == NO_SLOT)
    {
        return NULL;
    }
    fx = bindSlot(fx_spawnTimedOnActor(on, frames, aura, slot, layer), slot);
    if (fx != NULL)
    {
        fx->type = ACTOR_TYPE_UNKNOWN_50;
        fx->tickCallback = (ActorCallback)(void *)fx_tickTimedAttachment;
    }
    return fx;
}

void hackLib_fxDetach(Actor *fx, const Vector2Fp16 *pos)
{
    Vector2Fp16 *spawnPos = (Vector2Fp16 *)((u8 *)&fx->as + FX_SPAWN_POS_OFFSET);
    Vector2Fp16 at = *pos;

    if (isHoop(fx))
    {
        at.y -= FX_HOOP_LIFT << 16;
    }
    fx->renderFollowTarget = NULL;
    fx->spriteFlags &= ~SPRITE_FLAGS_FOLLOW_LAYER;
    fx->coords = at;
    fx->renderCoords = at;
    *spawnPos = at;
}

Actor *hackLib_fxAt(Actor *origin, const Vector2Fp16 *pos, u8 shape, HackLibFxColor color)
{
    if (pos == NULL)
    {
        return NULL;
    }
    Actor *fx = hackLib_fxBurst(origin, shape, color, FX_LAYER_BEHIND);

    if (fx != NULL)
    {
        hackLib_fxDetach(fx, pos);
    }
    return fx;
}

Actor *hackLib_fxSprite(Actor *on, const SpriteAnimation *animation, HackLibFxColor color,
                        u8 frameTicks, u8 layer)
{
    if (!validAnimation(animation, frameTicks))
    {
        return NULL;
    }
    Actor *fx = hackLib_fxBurst(on, FX_SHAPE_GROUND_POOL, color, layer);

    if (fx != NULL)
    {
        sprite_setAnimation(fx, animation);
    }
    return hackLib_fxPace(fx, frameTicks);
}

Actor *hackLib_fxEnemyAnim(Actor *on, u8 enemyType, u8 pose, u8 frameTicks, u8 layer)
{
    if (enemyType >= ENEMY_TYPE_COUNT || pose >= ENEMY_ANIMATION_POSE_COUNT)
    {
        return NULL;
    }
    return hackLib_fxSprite(on, EnemyAnimationBanks[enemyType][pose],
                            FX_COLOR_OBJ_PALETTE(ENEMY_PALETTE_IDS[enemyType]), frameTicks, layer);
}

Actor *hackLib_fxSpritePalette(Actor *on, const SpriteAnimation *animation, const u16 *palette,
                               u8 paletteId, u8 frameTicks, u8 layer)
{
    if (!validRequest(on, FX_SHAPE_GROUND_POOL, layer) || !validAnimation(animation, frameTicks) ||
        animation->palette != NULL || palette == NULL || paletteId >= 32)
    {
        return NULL;
    }
    u8 slot = claimSlot(IMPORTED_TAG_BASE + paletteId, palette);
    if (slot == NO_SLOT)
    {
        return NULL;
    }
    Actor *fx = bindSlot(fx_spawnOnActor(on, FX_SHAPE_GROUND_POOL, slot, layer), slot);
    if (fx != NULL)
    {
        fx->type = FX_BURST_UNK_PARAM;
        fx->tickCallback = (ActorCallback)(void *)fx_tickAttachment;
        sprite_setAnimation(fx, animation);
        hackLib_fxPace(fx, frameTicks);
    }
    return fx;
}

Actor *hackLib_fxPace(Actor *fx, u8 frameTicks)
{
    if (fx != NULL)
    {
        fx->animationFrameDuration = frameTicks;
        fx->animationFramesLeft = frameTicks;
    }
    return fx;
}

void hackLib_fxTintSlot(u8 slot, const u16 *base, u8 first, u8 count, u16 color, u8 strength)
{
    u16 *out;
    u32 i;

    if (slot >= FX_TINT_SLOT_COUNT)
    {
        return;
    }
    out = tintedSlots[slot];
    memory_memcpy(out, base, sizeof(tintedSlots[0]));
    for (i = first; i < first + count && i < 16; i++)
    {
        s32 from = base[i];
        s32 mixed = 0;
        u32 shift;

        for (shift = 0; shift < 15; shift += 5)
        {
            s32 a = (from >> shift) & 0x1f;
            s32 b = (color >> shift) & 0x1f;

            mixed |= (a + (b - a) * strength / FX_TINT_FULL) << shift;
        }
        out[i] = mixed;
    }
    if (!(g_GameFlags.p & GAME_FLAG_OVERLAY) && g_PaletteWriteCount != 0)
    {
        loadSlot(slot, out);
        return;
    }
    const u16 *current = (g_GameFlags.p & GAME_FLAG_OVERLAY)
                             ? g_ObjPaletteSnapshot + slot * 16
                             : (const u16 *)OBJ_PALETTE_RAM + slot * 16;
    for (i = 0; i < 16; i++)
    {
        if ((current[i] & 0x7fff) != (out[i] & 0x7fff))
        {
            loadSlot(slot, out);
            break;
        }
    }
}

void hackLib_fxRemove(Actor *fx)
{
    fx->flags.p = (fx->flags.p & ~ACTOR_FLAG_RENDER) | ACTOR_FLAG_PENDING_REMOVE |
                  ACTOR_FLAG_ANIMATION_FRAME_CHANGED;
}

static void tickHold(Actor *fx)
{
    FxHold *hold = NULL;
    u32 i;

    for (i = 0; i < FX_HOLD_COUNT; i++)
    {
        if (holds[i].fx == fx && holds[i].generation == hackLib_actorGeneration(fx))
        {
            hold = &holds[i];
            break;
        }
    }
    // Entering the last frame raises ACTOR_FLAG_ANIMATION_END. A full countdown there keeps the
    // frame from advancing.
    if (hold != NULL && hold->framesLeft != 0 &&
        fx->currentAnimationFrame + 1 >= fx->spriteAnimation->frames->frameCount)
    {
        fx->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
        fx->animationFramesLeft = 0xff;
    }
    if (hold == NULL || hold->framesLeft == 0 || fx_tickAttachment(fx))
    {
        if (hold != NULL)
        {
            hold->fx = NULL;
        }
        hackLib_fxRemove(fx);
        return;
    }
    hold->framesLeft--;
}

bool32 hackLib_fxHold(Actor *fx, u16 frames)
{
    FxHold *free = NULL;
    u32 i;

    if (fx == NULL)
    {
        return FALSE;
    }
    for (i = 0; i < FX_HOLD_COUNT; i++)
    {
        FxHold *hold = &holds[i];

        // A hold whose actor was freed and reused no longer runs tickHold.
        if (hold->fx == fx || hold->fx == NULL ||
            !hackLib_isActorRefValid((HackLibActorRef){hold->fx, hold->generation}) ||
            hold->fx->tickCallback != (ActorCallback)tickHold)
        {
            free = hold;
            if (hold->fx == fx)
            {
                break;
            }
        }
    }
    if (free == NULL)
    {
        return FALSE;
    }
    free->fx = fx;
    free->generation = hackLib_actorGeneration(fx);
    free->framesLeft = frames;
    fx->tickCallback = (ActorCallback)tickHold;
    return TRUE;
}
