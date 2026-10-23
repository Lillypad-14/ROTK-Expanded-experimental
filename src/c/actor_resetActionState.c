#include "actor.h"
#include "player.h"
#include "sprite.h"
#include "variables.h"

/**
 * Undo the per-action-state flags, sprite slots, and aim FX of the state an actor is leaving.
 *
 * @romaddress 0x080257d0
 */
void actor_resetActionState(Actor *actor)
{
    u8 playerIndex = actor->as.combat.playerIndex;

    switch (actor->actionState)
    {
    case ACTOR_STATE_ARAGORN_SWEEP:
        sprite_detachSlot(actor, 3);
        actor->as.combat.actionStateFlags &= ~(1 << 0);
        break;
    case ACTOR_STATE_GIMLI_WHIRLING_ATTACK:
        sprite_detachSlot(actor, 3);
        actor->as.combat.actionStateFlags &= ~(1 << 1);
        break;
    case ACTOR_STATE_LEGOLAS_WHITE_KNIVES:
        actor->as.combat.actionStateFlags &= ~(1 << 0);
        actor->field_0x100 &= ~(1 << 5);
        sprite_detachSlot(actor, 1);
        sprite_detachSlot(actor, 2);
        break;
    case ACTOR_STATE_EOWYN_DOUBLE_STRIKE:
        actor->as.combat.actionStateFlags &= ~(1 << 0);
        sprite_detachSlot(actor, 3);
        break;
    case ACTOR_STATE_EOWYN_SHIELD_BASH:
        actor->as.combat.actionStateFlags &= ~(1 << 3);
        sprite_detachSlot(actor, 3);
        break;
    case ACTOR_STATE_SAM_COOKPOT_SMASH:
        actor->as.combat.actionStateFlags &= ~(1 << 3);
        break;
    case ACTOR_STATE_SMEAGOL_BERSERK_ATTACK:
        actor->as.combat.actionStateFlags &= ~(1 << 0);
        break;
    case 0x4a:
    case 0x4b:
    case 0x5a:
    case 0x5b:
    case 0x5c:
        actor->as.combat.actionStateFlags &= ~(1 << 2);
        break;
    case ACTOR_STATE_LEGOLAS_OVERDRAW:
        if (PLAYER(playerIndex).aimFxActor != NULL)
        {
            Actor *aimFx = PLAYER(playerIndex).aimFxActor;
            aimFx->flags.p = (aimFx->flags.p & ~ACTOR_FLAG_RENDER) | 0x82;
            PLAYER(playerIndex).bowHoldFrames = 0;
            PLAYER(playerIndex).aimFxActor = NULL;
        }
        break;
    case 1:
    default:
        break;
    }
    PLAYER(playerIndex).activeSkillStateFlags &= ~ACTIVE_SKILL_STATE_FLAG_CASTING;
}
