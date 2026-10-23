#include "actor.h"

/**
 * Knock victim away from attacker for duration frames.
 *
 * @romaddress 0x08009aac
 */
void actor_applyKnockback(Actor *victim, Actor *attacker, u8 duration)
{
    u8 direction;

    if (attacker->type == ACTOR_TYPE_UNKNOWN_00)
        direction = actor_direction8Toward(attacker, victim);
    else
        direction = attacker->facing;
    victim->actionState = ACTOR_STATE_KNOCKBACK;
    victim->actionStateTimer = duration;
    victim->field_0x64[0] |= 2;
    actor_setVelocityFromDirection8(&victim->velocity, victim->flags.p, direction, 0x20000);
}

/**
 * @romaddress 0x08009b04
 */
bool actor_unknown08009b04(Actor *actor, u8 arg)
{
    u8 a;
    u8 b;
    u8 c;

    actor_unknown08008558(actor, arg, &a, &b, &c);
    if ((u8)(a - 1) > 1 && (u8)(b - 1) > 1 && (u8)(c - 1) > 1)
        return FALSE;
    return TRUE;
}

/**
 * Stun the actor for duration frames if it can be stunned.
 *
 * @romaddress 0x08009b54
 */
void actor_stun(Actor *actor, u16 duration)
{
    if (!(actor->field_0x64[5] & 0x10) && actor_isStunnable(actor))
    {
        actor->actionState = ACTOR_STATE_STUNNED;
        actor->actionStateTimer = duration;
        actor->field_0x54 = duration;
        actor->field_0x64[0] |= 2;
        actor->field_0x64[5] |= 0x10;
        actor_clearVelocity(actor);
    }
    else
        actor->actionStateTimer = duration;
}
