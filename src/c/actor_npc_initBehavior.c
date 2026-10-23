#include "actor.h"

/**
 * Put an NPC actor into its initial invulnerable state.
 *
 * @romaddress 0x080364a4
 */
void actor_npc_initBehavior(Actor *actor)
{
    actor->behaviorState = 0;
    actor->actionStateTimer = 0;
    actor->actionState = (ActorActionState)0x12;
    actor->field_0x64[0] = 0;
    actor->flags.p |= ACTOR_FLAG_INVULNERABLE | ACTOR_FLAG_23;
}
