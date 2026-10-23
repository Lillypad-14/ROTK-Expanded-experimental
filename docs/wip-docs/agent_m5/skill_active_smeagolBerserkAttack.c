// header declares this void; the ROM returns 0 (change prototype to u32)
#define skill_active_smeagolBerserkAttack skill_active_smeagolBerserkAttack_hdr
#include "actor.h"
#include "player.h"
#include "sfx.h"
#include "skill.h"
#include "variables.h"
#undef skill_active_smeagolBerserkAttack

/**
 * Cast Berserk Attack.
 *
 * @romaddress 0x08044ec8
 */
u32 skill_active_smeagolBerserkAttack(Actor *playerActor)
{
    u32 playerIndex = playerActor->as.combat.playerIndex;
    s32 level = (s8)PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_SMEAGOL_BERSERK_ATTACK];

    playerActor->as.combat.head.player.animationBank =
        (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId];
    playerActor->as.combat.requestedPose = PLAYER(playerIndex).castingActiveSkill + 1;
    playerActor->actionState = ACTOR_STATE_SMEAGOL_BERSERK_ATTACK;
    playerActor->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
    playerActor->as.combat.storedDamage = skill_active_getLeveledValue(
        playerIndex, ACTIVE_SKILL_SMEAGOL_BERSERK_ATTACK, 0, level);
    playerActor->as.combat.actionStateFlags |= 1;
    sfx_play(0x82);
    return 0;
}
