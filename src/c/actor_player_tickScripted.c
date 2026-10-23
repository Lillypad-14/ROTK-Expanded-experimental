#include "actor.h"
#include "match_hacks.h"
#include "player.h"
#include "sprite.h"
#include "variables.h"

/**
 * Tick for a player actor under mission-script control, installed by actor_player_initScripted.
 * Runs the hero pose-1 animation until it ends, then drives the pose and velocity from the
 * state flags in field_0x64.
 *
 * @romaddress 0x08025a5c
 */
void actor_player_tickScripted(Actor *actor)
{
    u32 playerIndex = actor->as.combat.playerIndex;
    u32 heroId = PLAYER(playerIndex).heroId;
    u32 facing;
    u32 flags;
    u32 flags2;

    switch (actor->behaviorState)
    {
    case 4:
        if (heroId == 0)
        {
            sprite_selectAnimation(actor, (void *)HeroMoveAnimationBanks[heroId], 1,
                                   (s8)FacingVariants[actor->facing]);
            actor->flags.p = (actor->flags.p | (ACTOR_FLAG_19 | ACTOR_FLAG_ANIMATION_PLAYING)) &
                             ~(ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_TILE_COLLISION);
            actor->oam.bgPriority = 3;
            actor->behaviorState = 5;
            actor->field_0x64[0] = 0;
        }
        break;
    case 5:
        if (heroId == 0)
        {
            if (actor->flags.p & ACTOR_FLAG_ANIMATION_END)
            {
                actor->flags.p &= ~ACTOR_FLAG_ANIMATION_PLAYING;
                actor->behaviorState = 0;
                actor_clearVelocity(actor);
            }
        }
        break;
    default:
        actor_runBehaviorState(actor);
        break;
    }

    facing = actor->facing;
    flags = actor->field_0x64[0];
    flags2 = flags;
    if (flags & 4)
        actor_setVelocityFromDirection8(&actor->velocity, actor->flags.p, facing,
                                        actor->moveSpeed);
    if (flags & 8)
        sprite_selectAnimation(actor, (void *)HeroMoveAnimationBanks[heroId], 4,
                               (s8)FacingVariants[facing]);
    if ((flags & 1) || (flags2 & 2))
    {
        if (actor->actionState == 2)
        {
            sprite_selectAnimation(actor, (void *)HeroMoveAnimationBanks[heroId], 6,
                                   (s8)FacingVariants[facing]);
            actor_setVelocityFromDirection8(&actor->velocity, actor->flags.p, facing,
                                            actor->moveSpeed);
        }
        else
        {
            sprite_selectAnimation(actor, (void *)HeroMoveAnimationBanks[heroId], 4,
                                   (s8)FacingVariants[facing]);
            actor->velocity.x = 0;
            actor->velocity.y = 0;
        }
    }
    if (facing > 4)
        ((u8 *)&actor->oam)[3] |= 0x10;
    else
        { u8 *p = (u8 *)&actor->oam + 3; u32 v = *p; u32 m = ~0x10; MATCH_BARRIER(m); *p = m & v; }
    actor_updateBgPriorityFromTerrain(actor);
    actor->field_0x64[0] = 0;
}
