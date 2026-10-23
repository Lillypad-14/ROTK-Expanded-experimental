// header declares this void; the ROM returns 0 (change prototype to u32)
#define skill_active_gimliWhirlingAttack skill_active_gimliWhirlingAttack_hdr
#define sprite_selectAnimation sprite_selectAnimation_hdr
#include "actor.h"
#include "player.h"
#include "sfx.h"
#include "skill.h"
#include "sprite.h"
#include "variables.h"
#undef skill_active_gimliWhirlingAttack
#undef sprite_selectAnimation

// ROM passes pose and facing variant as s8
void sprite_selectAnimation(Actor *actor, void *animationBank, s8 pose, s8 facingVariant);

/**
 * Cast Whirling Attack.
 *
 * @romaddress 0x080447a4
 */
u32 skill_active_gimliWhirlingAttack(Actor *playerActor)
{
    u32 playerIndex = playerActor->as.combat.playerIndex;
    s32 level = (s8)PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_GIMLI_WHIRLING_ATTACK];

    playerActor->as.combat.head.player.animationBank =
        (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId];
    playerActor->as.combat.requestedPose = PLAYER(playerIndex).castingActiveSkill + 1;
    playerActor->animationFrameDuration = 2;
    sprite_selectAnimation(playerActor, (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId],
                           (s8)(PLAYER(playerIndex).castingActiveSkill + 1),
                           (s8)FacingVariants[playerActor->facing]);
    playerActor->actionState = ACTOR_STATE_GIMLI_WHIRLING_ATTACK;
    playerActor->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
    playerActor->as.combat.storedDamage = skill_active_getLeveledValue(
        playerIndex, ACTIVE_SKILL_GIMLI_WHIRLING_ATTACK, 0, level);
    playerActor->as.combat.actionStateFlags |= 2;
    sfx_play(0x4e);
    return 0;
}
