#include "actor.h"
#include "fx.h"
#include "player.h"
#include "sfx.h"
#include "skill.h"
#include "variables.h"

/**
 * Cast Axe Throw.
 *
 * @romaddress 0x080220e8
 */
Actor *skill_active_gimliAxeThrow(Actor *playerActor)
{
    u32 playerIndex = playerActor->as.combat.playerIndex;
    u8 level = PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_GIMLI_AXE_THROW];
    u8 damage = skill_active_getLeveledValue(playerIndex, ACTIVE_SKILL_GIMLI_AXE_THROW, 0, level);
    Actor *fx;

    if (level > 2)
        fx = fx_unknown08043d74(playerActor, 0, 2, 1, 2, GimliAxeThrowFxVariants[level - 1]);
    else
        fx = fx_unknown08043d74(playerActor, 0, 2, 0, 2, GimliAxeThrowFxVariants[level - 1]);

    fx->type = 0x40;
    fx->tickCallback = skill_active_gimliAxeThrow_tick;
    fx->triggerCallbacks[0] = actor_unknown0801653c;
    *(u16 *)&fx->as.combat.head.scripted.params[0] = damage;
    fx->parentActor = playerActor;
    playerActor->as.combat.actionStateFlags |= 1;
    sfx_play(0x4d);
    return fx;
}

/**
 * Per-frame tick of a thrown axe. Gloin's Double Axes throws a second one.
 *
 * @romaddress 0x0802219c
 */
void skill_active_gimliAxeThrow_tick(Actor *fx)
{
    u32 playerIndex = fx->parentActor->as.combat.playerIndex;

    if (fx->parentActor->actionState == ACTOR_STATE_UNKNOWN_2D)
        fx->actionState = ACTOR_STATE_UNKNOWN_04;
    if (fx->actionStateTimer == 0x18)
    {
        u32 index = playerIndex;
        u32 slot = skill_passive_findSlot(index, PASSIVE_SKILL_GLOINS_DOUBLE_AXES);

        if (slot != 9 && PLAYER(playerIndex).passiveSkillLevelsApplied[slot] > 0)
        {
            s32 level = (s8)PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_GIMLI_AXE_THROW];
            s32 damage = skill_active_getLeveledValue(
                index, ACTIVE_SKILL_GIMLI_AXE_THROW, 0,
                PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_GIMLI_AXE_THROW]);
            Actor *axe;

            if (level > 2)
                axe = fx_unknown08043d74(fx->parentActor, *(u32 *)&fx->as, 2,
                                         1, 2, GimliAxeThrowFxVariants[level - 1]);
            else
                axe = fx_unknown08043d74(fx->parentActor, *(u32 *)&fx->as, 2,
                                         0, 2, GimliAxeThrowFxVariants[level - 1]);
            axe->type = 0x40;
            axe->tickCallback = (ActorCallback)actor_unknown08045584;
            axe->triggerCallbacks[0] = actor_unknown0801653c;
            *(u16 *)&axe->as.combat.head.scripted.params[0] = damage;
            axe->parentActor = fx->parentActor;
        }
    }
    if (actor_unknown08045584(fx) == 1)
        fx->parentActor->as.combat.actionStateFlags &= ~1;
}
