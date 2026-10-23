#include "lib/combat_result.h"
#include "lib/combat_source.h"
#include "lib/movement.h"
#include "lib/status.h"
#include "lib/summon.h"
#include "lib/timer.h"
#include "terrain.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

ActorList g_ActorListMain;
u16 *g_TerrainCollisionBlocks;
u8 *g_TerrainCollisionCells;
static u8 floorCells[16][16];
static Actor owner, ghost, enemies[2];
static u32 ticks, effects;
static bool stunnable = TRUE;

void terrain_getWorldSize(Vector2Int *size)
{
    size->x = size->y = 128;
}

u8 terrain_getCollisionAt(s32 x, s32 y)
{
    assert(x >= 0 && x < 128 && y >= 0 && y < 128);
    return floorCells[y / 8][x / 8];
}

bool actor_isStunnable(Actor *actor)
{
    (void)actor;
    return stunnable;
}

Actor *fx_attachStatusEffect(Actor *parent, Actor *target, u16 duration, u8 kind, u8 arg4, u8 arg5,
                             u8 flagBit)
{
    (void)parent;
    (void)kind;
    (void)arg4;
    (void)arg5;
    (void)flagBit;
    assert(target->actionStateTimer == duration);
    effects++;
    return NULL;
}

void actor_npc_tickPassive(Actor *actor)
{
    assert(actor == &ghost);
    ticks++;
    if (ghostState(actor)->lifetime > 0)
    {
        ghostState(actor)->lifetime--;
    }
}

static bool32 isTarget(Actor *actor)
{
    return actor->currentHp > 0;
}

static Actor *nextEnemy(Actor *actor, Actor *cursor)
{
    assert(actor == &owner);
    return cursor != NULL ? cursor->next : &enemies[0];
}

static void checkDamage(void)
{
    const s32 cases[][5] = {
        {100, 60, TRUE, 40, FALSE},         {100, -100, TRUE, 100, TRUE},
        {100, 0, TRUE, 100, TRUE},          {100, 0, FALSE, 0, FALSE},
        {0, -10, TRUE, 0, FALSE},           {-10, -20, TRUE, 0, FALSE},
        {100, 120, TRUE, 0, FALSE},         {100, 100, TRUE, 0, FALSE},
        {32767, -32768, TRUE, 32767, TRUE},
    };
    u32 i;

    for (i = 0; i < ARRAY_COUNT(cases); i++)
    {
        HackLibDamageResult result = hackLib_damageResult(cases[i][0], cases[i][1], cases[i][2]);
        assert(result.hit == cases[i][2]);
        assert(result.damageDealt == cases[i][3]);
        assert(result.killed == cases[i][4]);
    }
}

static void checkStun(void)
{
    Actor actor = {0};

    actor.currentHp = 100;
    assert(!hackLib_applyStun(NULL, 40));
    assert(!hackLib_applyStun(&actor, 0));
    stunnable = FALSE;
    assert(!hackLib_applyStun(&actor, 40));
    stunnable = TRUE;
    assert(hackLib_applyStun(&actor, 90));
    assert(actor.actionState == ACTOR_STATE_STUNNED && actor.actionStateTimer == 90);
    assert(!hackLib_applyStun(&actor, 40));
    assert(!hackLib_applyStun(&actor, 90));
    assert(actor.actionStateTimer == 90 && effects == 1);
    assert(hackLib_applyStun(&actor, 120));
    assert(hackLib_applyStun(&actor, 100000));
    assert(actor.actionStateTimer == 65535 && effects == 3);
    actor.currentHp = 0;
    assert(!hackLib_applyStun(&actor, 40));
}

static void checkSummon(void)
{
    GhostState *state = ghostState(&ghost);
    u32 i;

    ghost.type = ACTOR_TYPE_NPC;
    ghost.variant = GHOST_NPC_KIND;
    ghost.parentActor = &owner;
    ghost.behaviorState = 0x20;
    state->lifetime = 600;
    enemies[0].next = &enemies[1];
    enemies[0].currentHp = enemies[1].currentHp = 100;
    enemies[0].coords.x = 30 * FP16_16_ONE;
    enemies[1].coords.x = 10 * FP16_16_ONE;
    g_ActorListMain.head = &ghost;
    assert(hackLib_nextOwnedGhost(&owner, NULL) == &ghost);
    assert(hackLib_nextOwnedGhost(&enemies[0], NULL) == NULL);
    for (i = 0; i < 599; i++)
    {
        ghost.actionState = GHOST_ACTION_ATTACK;
        ghost.flags.p |= ACTOR_FLAG_ANIMATION_END;
        hackLib_tickPersistentGhost(&ghost, isTarget, nextEnemy);
        assert(ghost.actionState == GHOST_ACTION_PURSUIT);
        assert(!(ghost.flags.p & ACTOR_FLAG_ANIMATION_END));
        assert(state->target == &enemies[1]);
    }
    assert(state->lifetime == 1 && ticks == 599);
    assert(hackLib_nextOwnedGhost(&owner, NULL) == NULL);
    ghost.actionState = GHOST_ACTION_DEATH;
    hackLib_tickPersistentGhost(&ghost, isTarget, nextEnemy);
    assert(ghost.actionState == GHOST_ACTION_DEATH && state->lifetime == 0);
    state->lifetime = 600;
    enemies[0].currentHp = enemies[1].currentHp = 0;
    hackLib_tickPersistentGhost(&ghost, isTarget, nextEnemy);
    assert(state->target == NULL && ghost.actionState == GHOST_ACTION_IDLE);
    enemies[0].currentHp = 100;
    hackLib_tickPersistentGhost(&ghost, isTarget, nextEnemy);
    assert(state->target == &enemies[0]);
    ghost.behaviorState = GHOST_BEHAVIOR_DESPAWNING;
    ghost.actionState = GHOST_ACTION_DEATH;
    assert(hackLib_nextOwnedGhost(&owner, NULL) == NULL);
    hackLib_tickPersistentGhost(&ghost, isTarget, nextEnemy);
    assert(ghost.actionState == GHOST_ACTION_DEATH);
}

static void checkMovement(void)
{
    Actor actor = {0};
    Vector2Fp16 preferred = {64 * FP16_16_ONE, 64 * FP16_16_ONE};
    Vector2Fp16 result;
    u16 blocks;

    actor.movementCollisionBox = (SpriteBounds){-6, 6, -4, 4};
    assert(!hackLib_canLandAt(&actor, &preferred));
    g_TerrainCollisionBlocks = &blocks;
    g_TerrainCollisionCells = &floorCells[0][0];
    assert(hackLib_findLanding(&actor, &preferred, &result));
    assert(result.x == preferred.x && result.y == preferred.y);
    floorCells[8][8] = 1;
    assert(!hackLib_canLandAt(&actor, &preferred));
    assert(hackLib_findLanding(&actor, &preferred, &result));
    assert(hackLib_canLandAt(&actor, &result));
    memset(floorCells, 2, sizeof(floorCells));
    assert(!hackLib_findLanding(&actor, &preferred, &result));
    memset(floorCells, 0, sizeof(floorCells));
    preferred.x = 0;
    assert(!hackLib_canLandAt(&actor, &preferred));
    preferred.x = 127 * FP16_16_ONE;
    assert(!hackLib_canLandAt(&actor, &preferred));
    assert(!hackLib_findLanding(NULL, &preferred, &result));
}

static void checkSources(void)
{
    HackLibDamageSource primary = {&owner, HACKLIB_DAMAGE_ABILITY, 7,
                                   0,      HACKLIB_ELEMENT_NONE,   FALSE};
    HackLibDamageSource secondary = {&ghost, HACKLIB_DAMAGE_SECONDARY, 7,
                                     1,      HACKLIB_ELEMENT_NONE,     FALSE};
    const HackLibDamageSource *saved;

    assert(hackLib_currentDamageSource() == NULL);
    saved = hackLib_swapDamageSource(&primary);
    assert(saved == NULL);
    assert(hackLib_swapDamageSource(&secondary) == &primary);
    assert(hackLib_currentDamageSource()->origin == &ghost);
    assert(hackLib_swapDamageSource(&primary) == &secondary);
    hackLib_swapDamageSource(saved);
    assert(hackLib_currentDamageSource() == NULL);
}

int main(void)
{
    u16 timer = 0;

    assert(!hackLib_tickTimer(&timer));
    assert(!hackLib_refreshTimer(&timer, -1));
    assert(hackLib_refreshTimer(&timer, 2));
    assert(!hackLib_refreshTimer(&timer, 1));
    assert(!hackLib_tickTimer(&timer));
    assert(hackLib_tickTimer(&timer));
    assert(!hackLib_tickTimer(&timer));
    assert(hackLib_refreshTimer(&timer, 100000));
    assert(!hackLib_refreshTimer(&timer, 100000));
    checkDamage();
    checkStun();
    checkSummon();
    checkMovement();
    checkSources();
    puts("PASS: damage, statuses, summons, safe landing and nested damage sources");
    return 0;
}
