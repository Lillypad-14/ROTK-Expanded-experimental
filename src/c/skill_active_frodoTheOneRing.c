#include "actor.h"
#include "gfx.h"
#include "player.h"
#include "scene.h"
#include "skill.h"
#include "variables.h"

/**
 * Cast The One Ring.
 *
 * @romaddress 0x080204a4
 */
Actor *skill_active_frodoTheOneRing(Actor *playerActor)
{
    u8 playerIndex = playerActor->as.combat.playerIndex;
    u8 level = PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_FRODO_THE_ONE_RING];
    s32 damageBonus = skill_active_getLeveledValue(playerIndex, ACTIVE_SKILL_FRODO_THE_ONE_RING, 0, level);
    s32 duration = skill_active_getLeveledValue(playerIndex, ACTIVE_SKILL_FRODO_THE_ONE_RING, 1, level);
    Actor *ring = actor_allocMain();
    u32 i;

    if (ring == NULL)
        return NULL;
    ring->type = 0x5f;
    ring->tickCallback = actor_oneRing_tick;
    ring->triggerCallbacks[0] = NULL;
    ring->as.combat.venomDuration = damageBonus;
    PLAYER(playerIndex).stats[STAT_DAMAGE_SLASH] += damageBonus;
    ring->actionStateTimer = duration;
    ring->flags.p = 0x10008;
    ring->parentActor = playerActor;
    playerActor->flags.p |= 1 << 23;
    playerActor->as.combat.actionStateFlags |= 8;
    if (playerIndex == ACTIVE_PLAYER_INDEX)
        transition_setEffect(8);
    else
        playerActor->spriteFlags |= 0x20;
    ring->actionState = 1;
    for (i = 0; i < g_PlayerGlobals.activePlayerCount; i++)
        player_addCorruption(i, 50);
    PLAYER(playerIndex).ringActor = ring;
    return ring;
}

/**
 * Tick for the One Ring actor.
 *
 * @romaddress 0x080205c4
 */
void actor_oneRing_tick(Actor *ring)
{
    Actor *parent;
    u8 playerIndex;

    if (ring->parentActor->actionState == 45 && ring->actionStateTimer > 30)
    {
        ring->actionStateTimer = 30;
        if (ring->actionState == 1)
            ring->actionState = 2;
    }
    playerIndex = ring->parentActor->as.combat.playerIndex;
    switch (ring->actionState)
    {
    case 1:
        ring->actionStateTimer--;
        ring->actionState = 2;
        break;
    case 2:
        if (ring->actionStateTimer-- == 0)
        {
            ring->actionState = 12;
            ring->actionStateTimer = 30;
            if (playerIndex == ACTIVE_PLAYER_INDEX)
                gfx_startPaletteFade(48, 208, 30, 1,
                                     SceneInfoTable[g_SceneCurrent.args[1]].fadeColor);
        }
        break;
    case 12:
        if (ring->actionStateTimer-- == 0)
            ring->actionState = 4;
        break;
    case 4:
    default:
        PLAYER(playerIndex).stats[STAT_DAMAGE_SLASH] -= ring->as.combat.venomDuration;
        PLAYER(playerIndex).ringActor = NULL;
        ring->flags.p = (ring->flags.p & ~1) | 0x82;
        parent = ring->parentActor;
        parent->flags.p &= ~(1 << 23);
        parent->as.combat.actionStateFlags &= ~8;
        if (playerIndex == ACTIVE_PLAYER_INDEX)
            scene_unknown0804a48c();
        else
            ring->parentActor->spriteFlags &= ~0x20;
        break;
    }
}
