// header declares this void; the ROM returns 0 (change prototype to u32)
#define skill_active_aragornSweep skill_active_aragornSweep_hdr
#define sprite_selectAnimation sprite_selectAnimation_hdr
#include "actor.h"
#include "player.h"
#include "sfx.h"
#include "skill.h"
#include "sprite.h"
#include "variables.h"
#undef skill_active_aragornSweep
#undef sprite_selectAnimation

// ROM passes pose and facing variant as s8
void sprite_selectAnimation(Actor *actor, void *animationBank, s8 pose, s8 facingVariant);
extern const u8 AragornSweepSlotPalettes[]; // 0x08287da8

/**
 * Cast Sweep.
 *
 * @romaddress 0x080446c0
 */
u32 skill_active_aragornSweep(Actor *playerActor)
{
    u32 playerIndex = playerActor->as.combat.playerIndex;
    s32 level = (s8)PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_ARAGORN_SWEEP];

    playerActor->as.combat.head.player.animationBank =
        (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId];
    playerActor->as.combat.requestedPose = PLAYER(playerIndex).castingActiveSkill + 1;
    playerActor->animationFrameDuration = 2;
    sprite_selectAnimation(playerActor, (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId],
                           (s8)(PLAYER(playerIndex).castingActiveSkill + 1),
                           (s8)FacingVariants[playerActor->facing]);
    sprite_attachSlot(playerActor, 3, 8, AragornSweepSlotPalettes[level - 1]);
    sprite_setSlotDrawOrder(playerActor, 3, 7);
    sprite_setSlotFlag0(playerActor, 3);
    playerActor->actionState = ACTOR_STATE_ARAGORN_SWEEP;
    playerActor->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
    playerActor->as.combat.storedDamage = skill_active_getLeveledValue(
        playerIndex, ACTIVE_SKILL_ARAGORN_SWEEP, 0, level);
    playerActor->as.combat.actionStateFlags |= 1;
    return 0;
}
