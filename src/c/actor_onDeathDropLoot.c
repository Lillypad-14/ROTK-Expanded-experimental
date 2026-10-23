#include "actor.h"
#include "actor/lootPile.h"
#include "item.h"
#include "loot.h"
#include "mission.h"
#include "player.h"
#include "stats.h"
#include "variables.h"

/**
 * Drop loot and fire the death link once a dying enemy's death animation ends.
 *
 * @romaddress 0x08046124
 */
void actor_onDeathDropLoot(Actor *self)
{
    Item item;
    u8 lootType;
    ActorMissionParams *params;

    switch ((s32)self->behaviorState)
    {
    case 1:
        if (!(self->flags.p & ACTOR_FLAG_ON_SCREEN))
            self->flags.p = (self->flags.p & ~1) | 0x82;
        break;
    case 3:
        if (self->flags.p & ACTOR_FLAG_ANIMATION_END)
        {
            self->flags.p = (self->flags.p & ~0x14) | 0x80008;
            self->behaviorState = 1;
            if (self->variant != 7)
            {
                if (loot_rollDropChance(self->unitType))
                {
                    lootType = loot_generateDrop(&item, self->lastHitByPlayerIndex,
                                                 PLAYER_STAT(self->lastHitByPlayerIndex, STAT_EXTRA_TREASURE_PERCENT),
                                                 self->unitType);
                    actor_lootPile_create(self->coords, lootType, item, DROP_DESPAWN_TIMER);
                }
            }
            self->oam.bgPriority = 3;
            if ((s32)self->flags.p < 0)
            {
                params = mission_script_getActorParams(self->recordGroup, self->recordIndex);
                mission_script_spawnGroupAndStartStream(((u8 *)params)[11], ((u8 *)params)[12]);
            }
        }
        break;
    case 0:
    default:
        break;
    }
    self->field_0x64[0] = 0;
}
