#include "actor.h"
#include "player.h"
#include "sfx.h"
#include "skill.h"
#include "sprite.h"
#include "variables.h"

/**
 * Cast White Knives.
 *
 * @romaddress 0x0804486c
 */
u32 skill_active_legolasWhiteKnives(Actor *playerActor)
{
    u32 playerIndex = playerActor->as.combat.playerIndex;
    s32 level = (s8)PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_LEGOLAS_WHITE_KNIVES];

    playerActor->as.combat.head.player.animationBank =
        (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId];
    playerActor->as.combat.requestedPose = 1;
    sprite_selectAnimation(playerActor, (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId],
                           1,
                           (s8)FacingVariants[playerActor->facing]);
    sprite_attachSlot(playerActor, 1, 2, 2);
    sprite_setSlotDrawOrder(playerActor, 1, 4);
    sprite_setSlotFlag1(playerActor, 1);
    sprite_attachSlot(playerActor, 2, 8, 7);
    sprite_setSlotDrawOrder(playerActor, 2, 5);
    playerActor->actionState = ACTOR_STATE_LEGOLAS_WHITE_KNIVES;
    playerActor->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
    playerActor->as.combat.storedDamage = skill_active_getLeveledValue(
        playerIndex, ACTIVE_SKILL_LEGOLAS_WHITE_KNIVES, 0, level);
    playerActor->as.combat.actionStateFlags |= 1;
    sfx_play(0x43);
    return 0;
}
