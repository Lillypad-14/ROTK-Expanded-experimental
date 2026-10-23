#include "actor.h"
#include "match_hacks.h"
#include "mission.h"

/**
 * Script op 36: put a stunnable actor into a scripted patrol state, picked by one of its flag bits.
 *
 * @romaddress 0x08017a84
 */
void mission_script_op36(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);
    u8 *p;
    u16 v;
    MATCH_PIN(u32 *, fp, "r1");
    MATCH_PIN(u32, f, "r2");

    if (actor && actor_isStunnable(actor))
    {
        if (actor->field_0x64[5] & 1)
        {
            actor->behaviorState = 29;
            actor->actionStateTimer = 0;
            actor->actionState = (ActorActionState)0x12;
            actor->field_0x64[0] = 0;
            p = &actor->as.combat.head.scripted.params[0];
            *p = 80;
            p -= 2;
            *(u16 *)p = 160;
            p += 3;
            *p = 10;
            p++;
            *p = 200;
            p += 7;
            *p = 40;
        }
        else
        {
            actor->behaviorState = 20;
            actor->actionStateTimer = 0;
            actor->actionState = (ActorActionState)0x12;
            actor->field_0x64[0] = 0;
            p = &actor->as.combat.head.scripted.params[0];
            *p = 35;
            v = *(const u16 *)(command + 6);
            p -= 2;
            *(u16 *)p = v;
            p += 3;
            *p = 10;
            p++;
            *p = 200;
        }
        fp = &actor->flags.p;
        f = *fp;
        f |= 0x204;
        *fp = f;
        if (!(f & 0x4000))
        {
            f &= ~ACTOR_FLAG_TICK_ALWAYS;
            *fp = f;
        }
    }
}
