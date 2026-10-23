#include "fx.h"
#include "player.h"
#include "sprite.h"
#include "variables.h"

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
