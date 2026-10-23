#include "fx.h"
#include "game.h"
#include "math.h"
#include "variables.h"

/**
 * Advance a particle's animation and position, then render or retire it.
 *
 * @return The signed rendering status, or zero when no draw is attempted.
 *
 * @romaddress 0x080387c4
 */
s32 fx_tickAndRenderParticle(FxParticle *particle)
{
    s32 result = 0;

    if (--particle->lifetime == -1)
    {
        fx_freeParticle((Actor *)particle);
    }
    else
    {
        u32 visibility;

        switch (particle->updateMode)
        {
        case 0:
            fx_particle_tickMode0(particle);
            break;
        case 1:
            fx_particle_tickMode1(particle);
            break;
        case 2:
            fx_particle_tickMode2(particle);
            break;
        default:
            fx_particle_tickMode2(particle);
            break;
        }
        if (particle->flags & FX_PARTICLE_FLAG_DIRECTIONAL_OSCILLATION)
        {
            u32 direction = ParticleOscillationDirections[particle->direction];
            const Direction8VectorTable *table = (const Direction8VectorTable *)Direction8MoveVectors;
            fp16_16 component = table->vectors[direction].y;
            fp16_16 offset = math_multiplyFp16(component, math_sinFp16(particle->phase));

            particle->renderPosition.x = particle->position.x + offset * particle->amplitude;
            component = table->vectors[direction].x;
            offset = math_multiplyFp16(component, math_sinFp16(particle->phase));
            particle->renderPosition.y = particle->position.y + offset * particle->amplitude;
            particle->phase = particle->phaseStep + particle->phase;
        }
        else if (particle->flags & FX_PARTICLE_FLAG_ORBIT)
        {
            particle->renderPosition.x = particle->position.x + particle->amplitude * math_cosFp16(particle->phase);
            particle->renderPosition.y = particle->position.y + particle->amplitude * math_sinFp16(particle->phase);
            particle->phase = particle->phaseStep + particle->phase;
        }
        else
        {
            particle->renderPosition.x = particle->position.x;
            particle->renderPosition.y = particle->position.y;
        }
        if (!(particle->flags & FX_PARTICLE_FLAG_ALWAYS_VISIBLE))
        {
            visibility = fx_classifyParticleVisibility(particle);
        }
        else
        {
            visibility = 0;
        }
        if (!(g_GameFlags.p & (GAME_FLAG_18 | GAME_FLAG_OVERLAY)))
        {
            if (visibility != 2)
            {
                result = (s8)fx_drawParticle(particle);
                if (result == -1)
                {
                    g_FxParticleRenderFailed = TRUE;
                    return result;
                }
            }
            else
            {
                fx_freeParticle((Actor *)particle);
            }
        }
    }
    return result;
}
