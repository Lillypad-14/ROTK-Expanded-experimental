#include "fx.h"
#include "player.h"
#include "sfx.h"
#include "skill.h"
#include "variables.h"

#define DWARVEN_RAGE_DURATION_STEP_FRAMES 60

/**
 * Activate Dwarven Rage and spawn its timed effects.
 *
 * @return The buff actor.
 *
 * @romaddress 0x080222a4
 */
Actor *skill_active_gimliDwarvenRage(Actor *playerActor)
{
    u32 playerIndex = playerActor->as.combat.playerIndex;
    s32 level = PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_GIMLI_DWARVEN_RAGE];
    Player *player = &PLAYER(playerIndex);
    s32 duration =
        (s16)(DWARVEN_RAGE_DURATION_STEP_FRAMES * level + DWARVEN_RAGE_DURATION_STEP_FRAMES);
    const u8 *variants = DwarvenRageFxVariants;
    const u8 *variant;
    Actor *buff;
    Actor *visual;
    Actor *castVisual;

    level--;
    variant = &variants[level];

    buff = fx_spawnTimedOnActor(playerActor, duration, 3, *variant, 1);
    visual = fx_spawnTimedOnActor(playerActor, duration, 8, *variant, 0);
    visual->tickCallback = (ActorCallback)fx_tickTimedAttachment;
    visual->type = ACTOR_TYPE_UNKNOWN_50;

    castVisual = fx_spawnOnActor(playerActor, 7, *variant, 0);
    castVisual->tickCallback = (ActorCallback)fx_tickAttachment;
    castVisual->type = ACTOR_TYPE_UNKNOWN_4F;

    buff->type = ACTOR_TYPE_UNKNOWN_3E;
    buff->flags.p |= ACTOR_FLAG_HIT_FLASH;
    visual->flags.p |= ACTOR_FLAG_HIT_FLASH;
    castVisual->flags.p |= ACTOR_FLAG_HIT_FLASH;

    // The buff's expiry also clears the player's combat flag.
    buff->tickCallback = skill_active_tickGimliDwarvenRage;
    buff->triggerCallbacks[0] = NULL;
    player->combatFlags.p |= PLAYER_COMBAT_FLAG_DWARVEN_RAGE;
    buff->parentActor = playerActor;

    playerActor->as.combat.actionStateFlags |= 4;
    sfx_play(SFX_DWARVEN_RAGE);
    return buff;
}

/**
 * Expire Dwarven Rage when its timed effect finishes or the caster dies.
 *
 * @romaddress 0x080223a0
 */
void skill_active_tickGimliDwarvenRage(Actor *actor)
{
    if (actor->parentActor->actionState == ACTOR_STATE_UNKNOWN_2D)
    {
        actor->actionState = ACTOR_STATE_UNKNOWN_04;
    }
    if (fx_tickTimedAttachment(actor) == TRUE)
    {
        Actor *parent;
        u32 playerIndex;

        actor->flags.p |= ACTOR_FLAG_PENDING_REMOVE | ACTOR_FLAG_ANIMATION_FRAME_CHANGED;
        parent = actor->parentActor;
        playerIndex = parent->as.combat.playerIndex;
        parent->as.combat.actionStateFlags &= ~4;
        PLAYER(playerIndex).combatFlags.p &= ~PLAYER_COMBAT_FLAG_DWARVEN_RAGE;
    }
}
