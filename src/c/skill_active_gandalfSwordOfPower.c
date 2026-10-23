#include "actor.h"
#include "player.h"
#include "skill.h"
#include "sprite.h"
#include "variables.h"

/**
 * Cast Sword of Power.
 *
 * @romaddress 0x08044c90
 */
u32 skill_active_gandalfSwordOfPower(Actor *playerActor)
{
    u32 playerIndex = playerActor->as.combat.playerIndex;
    s32 level = (s8)PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_GANDALF_SWORD_OF_POWER];

    playerActor->as.combat.head.player.animationBank =
        (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId];
    playerActor->as.combat.requestedPose = PLAYER(playerIndex).castingActiveSkill + 1;
    sprite_selectAnimation(playerActor, (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId],
                           (s8)(PLAYER(playerIndex).castingActiveSkill + 1),
                           (s8)FacingVariants[playerActor->facing]);
    playerActor->actionState = ACTOR_STATE_GANDALF_SWORD_OF_POWER;
    playerActor->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
    skill_active_getLeveledValue(playerIndex, ACTIVE_SKILL_GANDALF_SWORD_OF_POWER, 0, level);
    playerActor->as.combat.actionStateFlags |= 1;
    return 0;
}
