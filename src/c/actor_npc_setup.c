#include "actor.h"
#include "match_hacks.h"
#include "player.h"
#include "sprite.h"
#include "variables.h"

/**
 * Finish an ACTOR_TYPE_NPC actor: resolve the companion kinds to the active hero, pick the
 * tick (hero-bodied kinds use actor_npc_tick, others actor_npc_tickPassive), and wire the trigger
 * callbacks.
 *
 * @romaddress 0x08035940
 */
void actor_npc_setup(Actor *actor)
{
    u32 variant = actor->variant;
    u32 heroId = PLAYER(0).heroId;

    actor->flags.p |= ACTOR_FLAG_RENDER | ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_TICK_ALWAYS |
                      ACTOR_FLAG_ANIMATION_PLAYING | ACTOR_FLAG_RADIUS_PROXIMITY;
    sprite_setAnimation(actor, (SpriteAnimation *)NpcAnimationSets[actor->variant][3] +
                                   FacingVariants[actor->facing]);
    if ((u8)(variant - 25) > 1)
    {
        actor->flags.p |= ACTOR_FLAG_HIT_FLASH | ACTOR_FLAG_TILE_COLLISION;
    }
    else
    {
        actor->spriteFlags |= 0x20;
        if (variant == 26)
            actor->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
    }
    actor->oam.objMode = 0;
    actor->oam.colors256 = 0;
    actor->oam.bgPriority = 2;
    if (variant == 20)
        actor->oam.bgPriority = 1;
    actor->moveSpeed = 0x38000;
    if (variant == 10)
    {
        switch (heroId)
        {
        case HERO_ID_FRODO:
            variant = 6;
            break;
        case HERO_ID_LEGOLAS:
            variant = 5;
            break;
        case HERO_ID_GANDALF:
            variant = 8;
            break;
        case HERO_ID_EOWYN:
            variant = 7;
            break;
        case HERO_ID_GIMLI:
            variant = 2;
            break;
        default:
            variant = 1;
            break;
        }
        actor->variant = variant;
    }
    else if (variant == 11)
    {
        switch (heroId)
        {
        case HERO_ID_LEGOLAS:
            variant = 2;
            break;
        case HERO_ID_GANDALF:
            variant = 7;
            break;
        case HERO_ID_EOWYN:
            variant = 8;
            break;
        case HERO_ID_GIMLI:
            variant = 1;
            break;
        case HERO_ID_FRODO:
            variant = 9;
            break;
        default:
            variant = 5;
            break;
        }
        actor->variant = variant;
    }
    actor_applyTypeDefaults(actor);
    if ((u8)(variant - 7) < 2)
    {
        variant = 0;
        actor->variant = 0;
    }
    actor_updateBgPriorityFromTerrain(actor);
    if (actor->facing > 4)
        ((u8 *)&actor->oam)[3] |= 0x10;
    else
        {
            u32 v = ((u8 *)&actor->oam)[3];
            u32 m = ~0x10;
            MATCH_BARRIER(m);
            ((u8 *)&actor->oam)[3] = m & v;
        }
    if (variant < 7)
    {
        sprite_initSlots(actor);
        sprite_selectAnimation(actor, (void *)HeroMoveAnimationBanks[variant], 4,
                               (s8)FacingVariants[actor->facing]);
        sprite_attachSlot(actor, 0, 1, 2);
        sprite_setSlotDrawOrder(actor, 0, 5);
        sprite_setSlotFlag1(actor, 0);
        if (variant == 3)
        {
            sprite_attachSlot(actor, 1, 2, 2);
            sprite_setSlotDrawOrder(actor, 1, 4);
        }
        actor->tickCallback = actor_npc_tick;
    }
    else
    {
        actor->tickCallback = actor_npc_tickPassive;
    }
    if (variant != 26)
        actor->triggerCallbacks[1] = actor_npcGhost_trigger;
    if (variant != 22 && variant != 26)
    {
        actor->interactionRadius = 10;
        actor->triggerCallbacks[0] = actor_resolveTouch;
    }
    if (variant == 1)
        actor->combatFlags.p |= 1;
    actor->currentHp = 100;
    if (actor->variant == 20)
        actor->oam.bgPriority = 1;
    actor_npc_initBehavior(actor);
}
