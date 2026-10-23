#include "actor.h"
#include "enemy.h"
#include "loot.h"
#include "sfx.h"
#include "variables.h"

/**
 * Enter the death or hurt state once an actor's HP drops to zero.
 *
 * @romaddress 0x0800a54c
 */
void actor_checkDeath(Actor *actor)
{
    u32 state = actor->actionState;

    if (state == 6 || state == 0x11 || state == 4 || state == 0x15 || state == 0x1b || state == 0x13)
        return;
    if (actor->currentHp > 0)
        return;
    if (actor->type == 0x26)
    {
        actor->actionState = ACTOR_STATE_HURT;
        actor->field_0x64[0] |= 2;
        actor->flags.p &= ~(ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_TILE_COLLISION);
    }
    else if (actor->type == 0x29)
    {
        if ((u32)(state - 0x30) > 1 && state != 0x22 && state != 0xe)
        {
            actor->actionState = ACTOR_STATE_HURT;
            actor->field_0x64[0] |= 2;
            actor->flags.p |= ACTOR_FLAG_COLLISION_CHECK;
        }
    }
    else
    {
        if (actor->flags.p & 0x10040400)
            sfx_play(EnemySfxSets[actor->unitType].deathSfx);
        else if (actor->type == ACTOR_TYPE_NPC)
            sfx_play(NpcSfxSets[actor->variant].field_0x4);
        loot_rollDrop(actor);
        actor->actionState = 0x11;
        actor->flags.p |= ACTOR_FLAG_TICK_ALWAYS;
        actor->field_0x64[0] |= 2;
        actor->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
    }
}
