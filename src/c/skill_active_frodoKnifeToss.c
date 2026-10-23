#include "actor.h"
#include "fx.h"
#include "player.h"
#include "sfx.h"
#include "skill.h"
#include "variables.h"

/**
 * Cast Knife Toss (also Sam's).
 *
 * @romaddress 0x080203bc
 */
Actor *skill_active_frodoKnifeToss(Actor *playerActor)
{
    u32 playerIndex = playerActor->as.combat.playerIndex;
    u32 level = PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_FRODO_KNIFE_TOSS];
    u8 damage = skill_active_getLeveledValue(playerIndex, ACTIVE_SKILL_FRODO_KNIFE_TOSS, 0, level);
    Actor *fx = fx_unknown08043d74(playerActor, 0, 1, 1, 2, KnifeTossFxVariants[level - 1]);

    fx->type = 0x5e;
    fx->tickCallback = skill_active_frodoKnifeToss_tick;
    fx->triggerCallbacks[0] = actor_unknown080169b4;
    *(u16 *)&fx->as.combat.head.scripted.params[0] = damage;
    fx->parentActor = playerActor;
    if (level <= 2)
        playerActor->as.combat.actionStateFlags |= 1;
    if (PLAYER(playerIndex).heroId == HERO_ID_FRODO)
        sfx_play(0x6e);
    else
        sfx_play(0x7a);
    return fx;
}

/**
 * Per-frame tick of a thrown knife.
 *
 * @romaddress 0x08020470
 */
void skill_active_frodoKnifeToss_tick(Actor *fx)
{
    if (fx->parentActor->actionState == ACTOR_STATE_UNKNOWN_2D)
        fx->actionState = ACTOR_STATE_UNKNOWN_04;
    if (actor_unknown08045584(fx) == 1)
        fx->parentActor->as.combat.actionStateFlags &= ~1;
}
