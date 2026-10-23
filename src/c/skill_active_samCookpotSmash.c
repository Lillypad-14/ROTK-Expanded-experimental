#include "actor.h"
#include "player.h"
#include "sfx.h"
#include "skill.h"
#include "sprite.h"
#include "variables.h"

/**
 * Cast Cookpot Smash.
 *
 * @romaddress 0x08044e08
 */
u32 skill_active_samCookpotSmash(Actor *playerActor)
{
    u32 playerIndex = playerActor->as.combat.playerIndex;
    s32 level = (s8)PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_SAM_COOKPOT_SMASH];

    playerActor->as.combat.head.player.animationBank =
        (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId];
    playerActor->as.combat.requestedPose = PLAYER(playerIndex).castingActiveSkill + 1;
    sprite_selectAnimation(playerActor, (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId],
                           (s8)(PLAYER(playerIndex).castingActiveSkill + 1),
                           (s8)FacingVariants[playerActor->facing]);
    sprite_attachSlot(playerActor, 0, 1, 2);
    sprite_setSlotFlag1(playerActor, 0);
    playerActor->actionState = ACTOR_STATE_SAM_COOKPOT_SMASH;
    playerActor->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
    playerActor->as.combat.storedDamage = level;
    playerActor->as.combat.actionStateFlags |= 8;
    sfx_play(0x7c);
    return 0;
}
