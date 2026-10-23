#include "actor.h"
#include "enemy.h"
#include "math.h"
#include "mission.h"
#include "scene.h"
#include "sfx.h"
#include "variables.h"

/**
 * @romaddress 0x0800a64c
 */
void actor_unknown0800a64c(Actor *self)
{
    if (g_GameFlags.p & GAME_FLAG_SCENE_CHANGE)
    {
        scene_requestInGameDialog(*(u16 *)self->field_0x58);
        self->field_0x64[0] |= 2;
        mission_script_spawnGroupAndStartStream(self->behaviorLinkGroup, self->behaviorLinkStream);
    }
}

/**
 * Whether any other live actor in the main list is within radius of self.
 *
 * @romaddress 0x0800a68c
 */
bool actor_unknown0800a68c(Actor *self, u8 radius)
{
    Actor *other = g_ActorListMain.head;
    Vector2Int pos = *(Vector2Int *)&self->coords;

    while (other != NULL)
    {
        if ((other->flags.p & 0x10041400) && other != self)
        {
            if (((s32)math_vector2_distance(pos, *(Vector2Int *)&other->coords) >> 16) < radius)
                return TRUE;
        }
        other = other->next;
    }
    return FALSE;
}

/**
 * @romaddress 0x0800a6f4
 */
void actor_unknown0800a6f4(Actor *self)
{
    u32 dist;
    Actor *target = actor_unknown08008b74(self, &dist);
    Vector2Fp16 delta;

    if (dist < self->as.combat.head.scripted.params[0] && self->actionState != ACTOR_STATE_UNKNOWN_03)
    {
        sfx_play(EnemySfxSets[self->unitType].attackSfx);
        self->actionStateTimer = 16;
        self->actionState = ACTOR_STATE_UNKNOWN_03;
        self->facing = actor_direction8Toward(self, target);
        delta.x = target->coords.x - self->coords.x;
        delta.y = target->coords.y - self->coords.y;
        self->as.combat.head.scripted.params[7] = math_vector2_direction32(delta);
        self->field_0x64[0] |= 2;
    }
}
