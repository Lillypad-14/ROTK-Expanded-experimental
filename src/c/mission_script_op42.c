#include "actor.h"
#include "match_hacks.h"
#include "actor/npc.h"
#include "mission.h"

/**
 * Script op 42: turn a stunnable actor into a ghost-style NPC in behavior state 32.
 *
 * @romaddress 0x08017ff4
 */
void mission_script_op42(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);
    u8 *fear;
    MATCH_PIN(u8 *, p, "r1");
    u32 zero;
    u8 *venom;

    if (actor && actor_isStunnable(actor))
    {
        p = (u8 *)&actor->behaviorState;
        zero = 0;
        *p = 32;
        actor->actionStateTimer = zero;
        p--;
        *p = 0x12;
        actor->field_0x64[0] = zero;
        fear = (u8 *)&actor->as.combat.fearDuration;
        *fear = 35;
        p += 50;
        *p = 7;
        p--;
        *p = 10;
        venom = (u8 *)&actor->as.combat.venomDuration;
        *venom = 13;
        *(u32 *)&actor->as.combat.head = zero;
        p -= 5;
        *(u16 *)p = 15;
        if (actor->type == ACTOR_TYPE_NPC && actor->variant == 1)
        {
            *fear = 70;
            *venom = 9;
        }
        else
            *fear = 35;
        actor->flags.p |= 0x204;
        actor->flags.p &= ~(ACTOR_FLAG_TICK_ALWAYS | ACTOR_FLAG_INVULNERABLE | ACTOR_FLAG_23);
        actor->triggerCallbacks[1] = actor_npcGhost_trigger;
    }
}
