#include "actor.h"
#include "mission.h"

/**
 * Scripted enemy trigger: when a target is inside the first radius and faced, switch to the scripted behavior.
 * Otherwise turn towards a target inside the second radius and pause.
 *
 * @romaddress 0x0800b248
 */
void actor_unknown0800b248(Actor *self)
{
    u32 dist;
    Actor *target = actor_unknown08008b74(self, &dist);
    u8 *innerPtr = &((u8 *)&self->as.combat.venomDuration)[1];
    u8 outerRadius = ((u8 *)&self->as.combat.venomDuration)[0];
    u32 faced;

    if ((s32)dist < *innerPtr && self->as.combat.head.enemyAiParams[7] == 0)
    {
        faced = actor_isFacingActor(self, target);
        if (faced)
        {
            u8 group = self->as.combat.field_0x8c[0];
            u8 stream = self->as.combat.field_0x8c[1];

            self->flags.p &= ~8;
            actor_initBehaviorByType(self);
            self->flags.p |= 0x204;
            mission_script_spawnGroupAndStartStream(group, stream);
        }
        else if ((s32)dist < outerRadius)
        {
            self->facing = actor_direction8Toward(self, target);
            self->velocity.x = faced;
            self->velocity.y = faced;
            self->actionState = 0xc;
            self->actionStateTimer = 30;
            self->field_0x64[0] |= 3;
        }
    }
}
