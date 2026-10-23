#include "actor/enemySpawner.h"
#include "actor.h"
#include "match_hacks.h"
#include "match_hacks.h"
#include "mission.h"
#include "sprite.h"

/**
 * Create an enemy spawner from its spawn-group params. The RNG variant (type 9) carries
 * the extra spawn id and probability bytes in a rearranged payload.
 *
 * @param groupIndex spawn group
 * @param actorIndex slot within the group
 * @return the new actor (NULL on pool exhaustion)
 *
 * @romaddress 0x08021c38
 */
Actor *actor_enemySpawner_createFromMission(u8 groupIndex, u8 actorIndex)
{
    u8 *params = (u8 *)mission_script_getActorParams(groupIndex, actorIndex);
    Actor *actor = actor_allocMain();
    u8 budget;
    u8 i;

    if (actor != NULL)
    {
        sprite_setPosition(actor, (s32)((ActorMissionParams *)params)->x << 16,
                           (s32)((ActorMissionParams *)params)->y << 16);
        actor->type = ((ActorMissionParams *)params)->type;
        actor->flags.p = ACTOR_FLAG_TICK_ALWAYS | ACTOR_FLAG_TICK_ONLY | ACTOR_FLAG_COLLISION_PASSIVE;
        if (actor->type == ACTOR_TYPE_ENEMY_SPAWNER_RNG)
        {
            actor->as.enemySpawner.enemySpawnId = params[9];
            {
                u16 v = params[11];
                ((u16 *)&actor->as)[9] = v;
            }
            {
                u16 *q;
                u16 v = params[13];
                MATCH_FRESH(q, (u16 *)&actor->as.enemySpawner.field_0x96[2]);
                *q = v;
            }
            MATCH_BARRIER(actor);
            actor->linkGroup = params[16];
            actor->linkStream = params[17];
            if ((u8)(params[10] + params[12]) > 100)
            {
                params[10] = 33;
                params[12] = 33;
            }
            actor->as.enemySpawner.field_0x96[4] = params[10];
            actor->as.enemySpawner.field_0x96[5] = params[12];
            actor->variant = params[8];
            actor->as.enemySpawner.budget = params[15];
            budget = params[14];
        }
        else
        {
            actor->as.enemySpawner.enemySpawnId = params[9];
            actor->linkGroup = params[12];
            actor->linkStream = params[13];
            actor->variant = params[8];
            actor->as.enemySpawner.budget = params[11];
            budget = params[10];
        }
        if (*(u8 *)&actor->as.enemySpawner.budget == 0)
            actor->as.enemySpawner.unbudgeted = TRUE;
        else
            actor->as.enemySpawner.unbudgeted = FALSE;
        actor->as.enemySpawner.cap = budget + 1;
        for (i = 0; i < 4; i++)
            actor->as.enemySpawner.children[i] = NULL;
        actor->tickCallback = actor_enemySpawner_tick;
        actor->behaviorState = 0;
        actor->actionState = 5;
        actor->actionStateTimer = 15;
    }
    return actor;
}

/**
 * Spawner state machine: 5 = spawn while credits and budget remain, 0xc = wait for every
 * child to die, 4 = retired. A shutdown request (behaviorState 1) moves 5 to 0xc.
 *
 * @romaddress 0x08021d64
 */
void actor_enemySpawner_tick(Actor *actor)
{
    bool allDead = TRUE;
    u8 i;

    if (actor->behaviorState == 1)
        actor->actionState = 0xc;

    switch ((s32)actor->actionState)
    {
    case 5:
        if ((*(u32 *)&actor->as.enemySpawner.field_0x96[6] & 0xff00ff00) == 0)
        {
            actor->actionState = 0xc;
        }
        else if (actor->variant != 0)
        {
            if (actor->as.enemySpawner.cap != 0)
                actor_enemySpawner_trySpawn(actor);
        }
        else if (actor->as.enemySpawner.cap != 0 && !(actor->flags.p & ACTOR_FLAG_ON_SCREEN))
        {
            actor_enemySpawner_trySpawn(actor);
        }
        break;
    case 0xc:
        for (i = 0; i < 4; i++)
        {
            if (actor->as.enemySpawner.children[i] != NULL)
                allDead = FALSE;
        }
        if (allDead)
        {
            actor->actionState = 4;
            mission_script_spawnGroupAndStartStream(actor->linkGroup, actor->linkStream);
        }
        break;
    case 4:
        actor->flags.p |= ACTOR_FLAG_ANIMATION_FRAME_CHANGED | ACTOR_FLAG_PENDING_REMOVE;
        break;
    }
}
