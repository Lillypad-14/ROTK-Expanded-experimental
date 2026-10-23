#include "lib/skill_fx.h"

#include "skill_fx_assets.inc"

void hackLib_stopEffect(HackLibEffectHandle *handle)
{
    if (handle == NULL)
    {
        return;
    }
    for (u32 i = 0; i < HACKLIB_EFFECT_LAYER_LIMIT; i++)
    {
        if (hackLib_isActorRefValid(handle->actors[i]))
        {
            hackLib_fxRemove(handle->actors[i].actor);
        }
        handle->actors[i] = (HackLibActorRef){0};
    }
}

u32 hackLib_playEffectDefinition(const HackLibVisualEffect *definition, Actor *on,
                                 const Vector2Fp16 *position, HackLibEffectHandle *handle)
{
    u32 spawned = 0;
    hackLib_stopEffect(handle);
    if (definition == NULL || on == NULL || definition->layers == NULL ||
        definition->count > HACKLIB_EFFECT_LAYER_LIMIT)
    {
        return 0;
    }
    for (u32 i = 0; i < definition->count; i++)
    {
        const HackLibEffectLayer *layer = &definition->layers[i];
        Actor *fx;
        if (layer->animation != NULL)
        {
            fx = hackLib_fxSpritePalette(on, layer->animation, layer->palette, layer->paletteId,
                                         layer->frameTicks, layer->layer);
        }
        else
        {
            fx =
                layer->duration != 0
                    ? hackLib_fxTimed(on, layer->shape, layer->color, layer->duration, layer->layer)
                    : hackLib_fxBurst(on, layer->shape, layer->color, layer->layer);
            hackLib_fxPace(fx, layer->frameTicks);
        }
        if (fx != NULL)
        {
            if (layer->fixed || position != NULL)
            {
                hackLib_fxDetach(fx, position != NULL ? position : &on->coords);
            }
            if (handle != NULL)
            {
                handle->actors[i] = hackLib_actorRef(fx);
            }
            spawned++;
        }
    }
    return spawned;
}

u32 hackLib_playEffect(u16 id, Actor *on, const Vector2Fp16 *position, HackLibEffectHandle *handle)
{
    if (id >= HACKLIB_VISUAL_EFFECT_COUNT)
    {
        hackLib_stopEffect(handle);
        return 0;
    }
    return hackLib_playEffectDefinition(&HackLibVisualEffects[id], on, position, handle);
}
