#include "actor/enemySpawner.h"
#include "actor.h"
#include "match_hacks.h"
#include "mission.h"
#include "rng.h"
#include "sprite.h"
#include "variables.h"

/**
 * @romaddress 0x08021ed0
 */
Actor *actor_enemySpawner_spawnChild(u16 enemySpawnId, Actor *spawner)
{
    Actor *actor;
    u8 id = enemySpawnId;
    s32 dx = 0;
    s32 dy = 0;

    actor = actor_allocMain();
    if (actor != NULL)
    {
        s32 *waypoint;
        s32 *waypointY;

        actor_unknown08009dec(actor);
        if (spawner->variant == 0 && (u16)(spawner->type - 8) <= 1)
        {
            dx = rng_rollRange(-20, 20);
            dy = rng_rollRange(-20, 20);
        }
        actor->facing = 4;
        sprite_setPosition(actor, spawner->coords.x + (dx << 16), spawner->coords.y + (dy << 16));
        actor->parentActor = spawner;
        actor->type = EnemySpawnEntries[id].type;
        actor->variant = EnemySpawnEntries[id].variant;
        if (actor->type == 25 && actor->variant <= 2)
        {
            actor_unknown080370a4(actor);
        }
        else
        {
            actor->unitType = EnemySpawnEntries[id].unitType;
            actor_unknown0801b798(actor);
            if (actor->type == 26)
            {
                if (actor->variant <= 1)
                    actor_unknown0804955c(actor);
                else if ((u8)(actor->variant - 4) <= 1)
                    actor_unknown080495b8(actor);
            }
            else if (actor->type == 48)
            {
                actor_unknown0801b6a0(actor);
            }
        }
        if (spawner->variant == 1)
        {
            actor->behaviorState = 0x2f;
            waypoint = (s32 *)&actor->field_0xbc[8];
            *waypoint = actor->coords.x;
            MATCH_FRESH(waypointY, waypoint + 1);
            *waypointY = actor->coords.y + (15 << 16);
            actor->flags.p &= 0xffff7dfb;
        }
        else if (spawner->variant == 2)
        {
            u8 zero;

            actor->behaviorState = 0x2f;
            zero = 0;
            waypoint = (s32 *)&actor->field_0xbc[8];
            *waypoint = actor->coords.x;
            MATCH_FRESH(waypointY, waypoint + 1);
            *waypointY = actor->coords.y - (15 << 16);
            actor->flags.p &= 0xffff7dfb;
            actor->facing = zero;
        }
        if (actor->type == 31)
            actor_unknown080229d8(actor);
        if (g_CurrentMissionId == MISSION_ITHILIEN_BLACK_GATE && g_MissionVariant == 1)
            actor->flags.p |= ACTOR_FLAG_TICK_ALWAYS;
    }
    return actor;
}
