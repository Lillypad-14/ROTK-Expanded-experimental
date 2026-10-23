#include "fx.h"
#include "player.h"
#include "sprite.h"
#include "variables.h"

/**
 * Free one hand affix glow emitter and clear the matching sprite slot flag.
 *
 * @romaddress 0x08027394
 */
void player_clearHandFx(u32 playerIndex, u32 hand)
{
    Actor *actor = PLAYER(playerIndex).ownerActor;

    if (hand == 0)
    {
        if (PLAYER(playerIndex).handFxEmitters[0] != NULL)
        {
            fx_freeEmitter(PLAYER(playerIndex).handFxEmitters[0]);
            PLAYER(playerIndex).handFxEmitters[0] = NULL;
        }
    }
    else
    {
        if (PLAYER(playerIndex).handFxEmitters[1] != NULL)
        {
            fx_freeEmitter(PLAYER(playerIndex).handFxEmitters[1]);
            PLAYER(playerIndex).handFxEmitters[1] = NULL;
        }
    }
    sprite_clearSlotFlag0(actor, hand);
}
