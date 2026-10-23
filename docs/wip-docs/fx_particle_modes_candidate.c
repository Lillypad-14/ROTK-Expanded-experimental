#include "fx.h"

/**
 * Advance particle animation.
 *
 * @romaddress 0x08038f34
 */
void fx_particle_tickMode0(FxParticle *particle)
{
    if (++particle->animationElapsed >= particle->animationFrameDuration || particle->animationFrame == -1)
    {
        if (particle->animationFrame < particle->animation->frames->frameCount - 1)
        {
            particle->animationFrame++;
        }
        else if (particle->flags & FX_PARTICLE_FLAG_LOOP_ANIMATION)
        {
            particle->animationFrame = 0;
        }
        particle->animationElapsed = 0;
    }
}

/**
 * Advance particle animation and linear movement.
 *
 * @romaddress 0x08038f98
 */
void fx_particle_tickMode1(FxParticle *particle)
{
    if (++particle->animationElapsed >= particle->animationFrameDuration || particle->animationFrame == -1)
    {
        if (particle->animationFrame < particle->animation->frames->frameCount - 1)
        {
            particle->animationFrame++;
        }
        else if (particle->flags & FX_PARTICLE_FLAG_LOOP_ANIMATION)
        {
            particle->animationFrame = 0;
        }
        particle->animationElapsed = 0;
    }
    particle->position.x += particle->velocity.x;
    particle->position.y += particle->velocity.y;
}

/**
 * Advance particle animation and linear movement.
 *
 * @romaddress 0x0803900c
 */
void fx_particle_tickMode2(FxParticle *particle)
{
    if (++particle->animationElapsed >= particle->animationFrameDuration || particle->animationFrame == -1)
    {
        if (particle->animationFrame < particle->animation->frames->frameCount - 1)
        {
            particle->animationFrame++;
        }
        else if (particle->flags & FX_PARTICLE_FLAG_LOOP_ANIMATION)
        {
            particle->animationFrame = 0;
        }
        particle->animationElapsed = 0;
    }
    particle->position.x += particle->velocity.x;
    particle->position.y += particle->velocity.y;
}

