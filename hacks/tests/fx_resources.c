#include "fx.h"
#include "game.h"
#include "lib/actor_ref.h"
#include "lib/fx_kit.h"
#include "lib/fx_shockwave.h"
#include "lib/profile.h"
#include "variables.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

u8 testTags[16], testRefs[16];
GameFlags g_GameFlags;
ActorList g_ActorListMain;
u16 *g_ObjPaletteSnapshot;
u32 g_PaletteWriteCount;
static const u16 colors[16];
const u16 *ObjPalettes[90];
const SpriteAnimation *const EnemyAnimationBanks[ENEMY_TYPE_COUNT][ENEMY_ANIMATION_POSE_COUNT];
static Actor owner, effect;
static bool failAllocation;
static u32 allocations, writes;
static u32 nativeTimedSpawns, rewinds;

void *memory_memcpy(void *destination, const void *source, size_t count)
{
    return memcpy(destination, source, count);
}
void gfx_queuePaletteWrite(const u16 *palette, u32 first, u32 count)
{
    assert(palette != NULL && first < 256 && count == 16);
    writes++;
}
Actor *fx_spawnOnActor(Actor *on, u8 shape, u8 slot, u8 layer)
{
    assert(on == &owner && shape < FX_SHAPE_COUNT && slot < 16 && layer <= FX_LAYER_FRONT);
    allocations++;
    memset(&effect, 0, sizeof(effect));
    return failAllocation ? NULL : &effect;
}
Actor *fx_spawnTimedOnActor(Actor *on, s32 frames, u8 shape, u8 slot, u8 layer)
{
    nativeTimedSpawns++;
    assert(frames > 0 && shape < FX_AURA_COUNT);
    return fx_spawnOnActor(on, shape, slot, layer);
}
u32 fx_tickAttachment(Actor *actor)
{
    (void)actor;
    return 0;
}
bool32 fx_tickTimedAttachment(Actor *actor)
{
    (void)actor;
    return FALSE;
}
void sprite_setAnimation(Actor *actor, const SpriteAnimation *animation)
{
    actor->spriteAnimation = animation;
}
void sprite_setAnimationFrame(Actor *actor, u8 frameIndex)
{
    assert(frameIndex == 0);
    actor->currentAnimationFrame = frameIndex;
    actor->animationFramesLeft = actor->animationFrameDuration;
    rewinds++;
}
u32 hackLib_actorGeneration(Actor *actor)
{
    return actor != NULL;
}
bool hackLib_isActorRefValid(HackLibActorRef reference)
{
    return reference.actor != NULL;
}
HackLibProfileSpan hackLib_profileBegin(u8 section)
{
    return (HackLibProfileSpan){0, section, FALSE};
}
void hackLib_profileEnd(HackLibProfileSpan span)
{
    (void)span;
}
void hackLib_profileWork(u8 section, u32 units)
{
    (void)section;
    (void)units;
}

int main(void)
{
    for (u32 i = 0; i < ARRAY_COUNT(ObjPalettes); i++)
    {
        ObjPalettes[i] = colors;
    }
    memset(testTags, 0x5a, sizeof(testTags));
    // Invalid requests must leave both the actor allocator and palette tables untouched.
    assert(hackLib_fxBurst(NULL, 0, FX_COLOR_SHADOW, 0) == NULL);
    assert(hackLib_fxTimed(NULL, 0, FX_COLOR_SHADOW, 30, 0) == NULL);
    assert(hackLib_fxTimed(&owner, 0, FX_COLOR_SHADOW, 0, 0) == NULL);
    assert(hackLib_fxBurst(&owner, FX_SHAPE_COUNT, FX_COLOR_SHADOW, 0) == NULL);
    assert(hackLib_fxBurst(&owner, 0, FX_COLOR_SHADOW, 2) == NULL);
    assert(hackLib_fxAt(&owner, NULL, 0, FX_COLOR_SHADOW) == NULL);
    assert(hackLib_fxSprite(&owner, NULL, FX_COLOR_SHADOW, 1, 0) == NULL);
    assert(hackLib_fxEnemyAnim(&owner, 255, 0, 1, 0) == NULL);
    assert(hackLib_fxBurst(&owner, 0, FX_COLOR_OBJ_PALETTE(90), 0) == NULL);
    assert(hackLib_fxBurst(&owner, 0, 16, 0) == NULL);
    assert(hackLib_fxBurst(&owner, 0, 0x300, 0) == NULL);
    assert(hackLib_fxBurst(&owner, 0, 9, 0) == NULL);
    assert(allocations == 0 && writes == 0 && testRefs[9] == 0);

    SpriteFrameSetHeader frames = {.maxFrameTileBytes = 2305};
    SpriteAnimation animation = {.frames = &frames};
    assert(hackLib_fxSprite(&owner, &animation, FX_COLOR_SHADOW, 1, 0) == NULL);
    frames.maxFrameTileBytes = 32;
    assert(hackLib_fxSprite(&owner, &animation, FX_COLOR_SHADOW, 0, 0) == NULL);
    assert(hackLib_fxSpritePalette(&owner, &animation, colors, 32, 1, 0) == NULL);
    assert(hackLib_fxSpritePalette(&owner, &animation, colors, 0, 1, 2) == NULL);
    assert(hackLib_fxAura(&owner, 14, FX_COLOR_WHITE, 30, 0) == NULL);
    assert(hackLib_fxAura(&owner, 255, FX_COLOR_WHITE, 30, 0) == NULL);
    assert(nativeTimedSpawns == 0);
    assert(allocations == 0 && writes == 0);

    failAllocation = TRUE;
    assert(hackLib_fxSpritePalette(&owner, &animation, colors, 0, 1, 0) == NULL);
    assert(testTags[9] == 0x5a && testRefs[9] == 0);
    failAllocation = FALSE;
    assert(hackLib_fxSpritePalette(&owner, &animation, colors, 0, 1, 0) == &effect);
    assert(testRefs[9] == 1 && effect.spriteAnimation == &animation);
    // Simulate the native release before exercising the stock shape path.
    testTags[9] = 0x5a;
    testRefs[9] = 0;

    // Failed actor allocation returns a newly claimed slot to the free pool.
    failAllocation = TRUE;
    assert(hackLib_fxBurst(&owner, 0, FX_COLOR_SHADOW, 0) == NULL);
    assert(testTags[9] == 0x5a && testRefs[9] == 0);
    failAllocation = FALSE;
    assert(hackLib_fxBurst(&owner, 0, FX_COLOR_SHADOW, 0) == &effect);
    assert(testRefs[9] == 1 && (effect.flags.p & ACTOR_FLAG_DEFAULTS_APPLIED));
    assert(hackLib_fxBurst(&owner, 0, 9, 0) == &effect);
    assert(testRefs[9] == 2);
    failAllocation = TRUE;
    assert(hackLib_fxTimed(&owner, 0, FX_COLOR_SHADOW, 30, 0) == NULL);
    assert(testRefs[9] == 2 && testTags[9] != 0x5a);

    // Shared reference counts must never wrap and free an occupied slot.
    testRefs[9] = 255;
    u32 before = allocations;
    assert(hackLib_fxBurst(&owner, 0, FX_COLOR_SHADOW, 0) == NULL);
    assert(hackLib_fxBurst(&owner, 0, 9, 0) == NULL);
    assert(testRefs[9] == 255 && allocations == before);
    memset(testTags + 9, 1, 7);
    assert(hackLib_fxBurst(&owner, 0, FX_COLOR_MORGUL, 0) == NULL);
    assert(allocations == before);
    // Timed burst shapes must not index the unrelated native timed-effect table.
    memset(testTags + 9, 0x5a, 7);
    memset(testRefs + 9, 0, 7);
    failAllocation = FALSE;
    Actor *timed = hackLib_fxTimed(&owner, FX_SHAPE_DIAMOND_RING, FX_COLOR_WHITE, 3, 0);
    assert(timed == &effect && nativeTimedSpawns == 0);
    timed->flags.p = ACTOR_FLAG_RENDER | ACTOR_FLAG_ANIMATION_END;
    timed->tickCallback(timed);
    assert(rewinds == 1 && !(timed->flags.p & ACTOR_FLAG_PENDING_REMOVE));
    timed->tickCallback(timed);
    assert(!(timed->flags.p & ACTOR_FLAG_PENDING_REMOVE));
    timed->tickCallback(timed);
    assert(timed->flags.p & ACTOR_FLAG_PENDING_REMOVE);
    assert(hackLib_fxTimed(&owner, 0, FX_COLOR_WHITE, 32768, 0) == NULL);
    // Both stock half-wave requests must use one complete animation and one actor.
    for (u8 shape = FX_SHAPE_SHOCKWAVE; shape <= FX_SHAPE_SHOCKWAVE_ALT; shape++)
    {
        before = allocations;
        Actor *wave = hackLib_fxBurst(&owner, shape, FX_COLOR_WHITE, FX_LAYER_BEHIND);
        assert(wave == &effect && allocations == before + 1);
        assert(wave->spriteAnimation == &HackLibFxGroundShockwave);
        assert(wave->spriteAnimation->frames->frameCount == 17);
        assert(wave->spriteAnimation->frames->maxFrameTileBytes <= 2304);
        Vector2Fp16 position = {100 << 16, 80 << 16};
        hackLib_fxDetach(wave, &position);
        assert(wave->coords.x == position.x && wave->coords.y == position.y);
        assert(wave->renderFollowTarget == NULL);
    }
    puts("PASS: FX request validation, palette rollback, sharing and exhaustion");
    return 0;
}
