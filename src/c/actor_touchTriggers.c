#include "actor.h"
#include "mission.h"
#include "scene.h"
#include "variables.h"

/**
 * @romaddress 0x08016290
 */
void actor_spawnGroupOnTouch(Actor *self, Actor *touching)
{
    if (touching->type == ACTOR_TYPE_UNKNOWN_00)
    {
        self->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
        self->behaviorState = 4;
        mission_script_spawnGroupAndStartStream(self->behaviorLinkGroup, self->behaviorLinkStream);
    }
}

/**
 * @romaddress 0x080162c4
 */
void actor_dialogTrigger(Actor *self, Actor *touching)
{
    if (touching->type == ACTOR_TYPE_UNKNOWN_00)
    {
        if (g_PlayerKeysPressed[touching->as.combat.playerIndex] & 0x100)
        {
            self->velocity.x = self->velocity.y = 0;
            self->renderCoords = self->coords;
            self->field_0x64[0] |= 8;
            scene_requestInGameDialog(*(u16 *)self->field_0x58);
            mission_script_spawnGroupAndStartStream(self->behaviorLinkGroup,
                                                    self->behaviorLinkStream);
        }
        actor_alignTouching(self, touching);
    }
}

/**
 * @romaddress 0x0801633c
 */
void actor_setBehaviorStateOnTouch(Actor *self, Actor *touching)
{
    if (touching->type == ACTOR_TYPE_UNKNOWN_00)
        self->behaviorState = 1;
}
