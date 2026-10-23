#include "actor.h"

/**
 * Push a victim away from an attacker for a number of frames.
 *
 * @romaddress 0x08009aac
 */
void actor_applyKnockback(Actor *victim, Actor *attacker, u32 duration)
{
    u8 dir;

    if (attacker->type == 0)
    {
        dir = actor_direction8Toward(attacker, victim);
    }
    else
    {
        dir = attacker->facing;
    }
    victim->actionState = ACTOR_STATE_KNOCKBACK;
    victim->actionStateTimer = (u8)duration;
    victim->field_0x64[0] |= 2;
    actor_setVelocityFromDirection8(&victim->velocity, victim->flags.p, dir, 0x20000);
}
