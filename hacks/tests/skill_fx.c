#include "lib/skill_fx.h"
#include <assert.h>
#include <stdio.h>

static Actor actors[8], owner;
static u32 generations[8];
static u32 spawned, removed, detached, timed, sprites;
static bool fail;
static s32 failAt = -1;

HackLibActorRef hackLib_actorRef(Actor *actor)
{
    return (HackLibActorRef){actor, generations[actor - actors]};
}
bool hackLib_isActorRefValid(HackLibActorRef reference)
{
    return reference.actor && reference.generation &&
           reference.generation == generations[reference.actor - actors];
}
Actor *hackLib_fxBurst(Actor *on, u8 shape, HackLibFxColor color, u8 layer)
{
    (void)on;
    (void)shape;
    (void)color;
    (void)layer;
    if (fail || (s32)spawned == failAt)
    {
        return NULL;
    }
    assert(spawned < 8);
    generations[spawned] = spawned + 1;
    return &actors[spawned++];
}
Actor *hackLib_fxTimed(Actor *on, u8 shape, HackLibFxColor color, s32 frames, u8 layer)
{
    assert(frames == 60);
    timed++;
    return hackLib_fxBurst(on, shape, color, layer);
}
Actor *hackLib_fxSpritePalette(Actor *on, const SpriteAnimation *animation, const u16 *palette,
                               u8 paletteId, u8 ticks, u8 layer)
{
    (void)animation;
    (void)palette;
    (void)paletteId;
    (void)ticks;
    sprites++;
    return hackLib_fxBurst(on, 0, 0, layer);
}
Actor *hackLib_fxPace(Actor *fx, u8 ticks)
{
    (void)ticks;
    return fx;
}
void hackLib_fxDetach(Actor *fx, const Vector2Fp16 *position)
{
    detached++;
    fx->coords = *position;
}
void hackLib_fxRemove(Actor *fx)
{
    (void)fx;
    removed++;
}

int main(void)
{
    const HackLibEffectLayer layers[] = {
        {.shape = 2, .color = 2, .frameTicks = 2},
        {.shape = 3, .color = 2, .duration = 60, .frameTicks = 2, .fixed = TRUE},
    };
    const HackLibVisualEffect definition = {layers, 2};
    HackLibEffectHandle handle = {0};
    owner.coords = (Vector2Fp16){12, 34};
    assert(hackLib_playEffectDefinition(&definition, &owner, NULL, &handle) == 2);
    assert(spawned == 2 && detached == 1 && timed == 1);
    assert(actors[1].coords.x == 12 && actors[1].coords.y == 34);
    // A recycled actor slot must not be removed through an old effect handle.
    generations[0]++;
    hackLib_stopEffect(&handle);
    assert(removed == 1 && handle.actors[0].actor == NULL);
    fail = TRUE;
    assert(hackLib_playEffectDefinition(&definition, &owner, NULL, &handle) == 0);
    assert(handle.actors[0].actor == NULL);
    fail = FALSE;
    Vector2Fp16 position = {56, 78};
    assert(hackLib_playEffectDefinition(&definition, &owner, &position, &handle) == 2);
    assert(detached == 3 && actors[2].coords.x == 56);
    assert(hackLib_playEffect(65535, &owner, NULL, &handle) == 0 && removed == 3);
    assert(hackLib_playEffect(HACKLIB_EFFECT_PREVIEW_POOL, &owner, NULL, &handle) == 1);
    assert(sprites == 1);
    hackLib_stopEffect(&handle);
    assert(removed == 4);
    assert(hackLib_playEffectDefinition(&definition, NULL, NULL, NULL) == 0);
    failAt = spawned + 1;
    assert(hackLib_playEffectDefinition(&definition, &owner, NULL, &handle) == 1);
    assert(handle.actors[0].actor != NULL && handle.actors[1].actor == NULL);
    hackLib_stopEffect(&handle);
    puts("PASS: effect recipes, placement, allocation failure and generation-safe cleanup");
    return 0;
}
