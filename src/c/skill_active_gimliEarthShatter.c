#include "actor.h"
#include "fx.h"
#include "player.h"
#include "sfx.h"
#include "skill.h"
#include "variables.h"

/**
 * Cast Earth Shatter.
 *
 * @romaddress 0x08022404
 */
Actor *skill_active_gimliEarthShatter(Actor *playerActor)
{
    u8 playerIndex = playerActor->as.combat.playerIndex;
    u8 level = PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_GIMLI_EARTH_SHATTER];
    u16 damage = skill_active_getLeveledValue(playerIndex, ACTIVE_SKILL_GIMLI_EARTH_SHATTER, 0, level);
    u16 duration = skill_active_getLeveledValue(playerIndex, ACTIVE_SKILL_GIMLI_EARTH_SHATTER, 1, level);
    Actor *fx = fx_spawnOnActor(playerActor, 0, GimliEarthShatterFxVariants[level - 1], 0);

    fx->flags.p |= 0x8004;
    fx->interactionRadius = damage;
    fx->type = 0x41;
    fx->tickCallback = skill_active_gimliEarthShatter_tick;
    fx->triggerCallbacks[0] = actor_unknown080168a4;
    fx->as.combat.venomDuration = damage;
    fx->as.combat.fearDuration = duration;
    fx->parentActor = playerActor;
    playerActor->as.combat.actionStateFlags |= 0x10;
    gfx_startScreenShake(level * 2, 30, TRUE);
    sfx_play(0x51);
    return fx;
}

/**
 * Tick for the Earth Shatter shockwave.
 *
 * @romaddress 0x080224cc
 */
void skill_active_gimliEarthShatter_tick(Actor *fx)
{
    if (fx->parentActor->actionState == 45)
        fx->actionState = 4;
    if (fx_tickAttachment(fx) == TRUE)
        fx->parentActor->as.combat.actionStateFlags &= ~0x10;
}
