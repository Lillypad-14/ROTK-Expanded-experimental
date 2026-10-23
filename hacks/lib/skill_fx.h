#pragma once

#include "lib/actor_ref.h"
#include "lib/fx_kit.h"

#define HACKLIB_EFFECT_LAYER_LIMIT 4
#define HACKLIB_EFFECT_PALETTE_LIMIT 32

typedef struct HackLibEffectLayer
{
    const SpriteAnimation *animation;
    const u16 *palette;
    u16 color;
    u16 duration;
    u8 shape;
    u8 frameTicks;
    u8 layer;
    u8 paletteId;
    bool fixed;
} HackLibEffectLayer;

typedef struct HackLibVisualEffect
{
    const HackLibEffectLayer *layers;
    u8 count;
} HackLibVisualEffect;

typedef struct HackLibEffectHandle
{
    HackLibActorRef actors[HACKLIB_EFFECT_LAYER_LIMIT];
} HackLibEffectHandle;

// Initialize handles to zero. Playing into a handle first stops its previous effects.

// Effects are cosmetic. Allocation failure must not change ability outcomes.
u32 hackLib_playEffect(u16 id, Actor *on, const Vector2Fp16 *position, HackLibEffectHandle *handle);
void hackLib_stopEffect(HackLibEffectHandle *handle);
u32 hackLib_playEffectDefinition(const HackLibVisualEffect *definition, Actor *on,
                                 const Vector2Fp16 *position, HackLibEffectHandle *handle);
extern const HackLibVisualEffect HackLibVisualEffects[];
extern const char *const HackLibVisualEffectNames[];

#include "skill_fx_ids.h"
