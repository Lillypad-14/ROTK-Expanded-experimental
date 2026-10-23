#include "actor.h"
#include "fx.h"
#include "player.h"
#include "variables.h"

/**
 * Point the hand glow emitters' draw priority flags by the owner's facing.
 *
 * @romaddress 0x08027230
 */
void player_updateHandFxFacing(s32 playerIndex)
{
    Actor *owner = g_PlayerGlobals.players[playerIndex].ownerActor;
    FxEmitter *emitter = g_PlayerGlobals.players[playerIndex].handFxEmitters[0];
    u16 bits;
    u16 flags;

    if (emitter != NULL)
    {
        if (owner->facing == DIRECTION8_NORTH || owner->facing == DIRECTION8_NORTHWEST)
        {
            flags = emitter->field_0x2e & 0xbfff;
            bits = 0x100;
        }
        else
        {
            flags = emitter->field_0x2e & 0xfeff;
            bits = 0x4000;
        }
        emitter->field_0x2e = flags | bits;
    }
    emitter = g_PlayerGlobals.players[playerIndex].handFxEmitters[1];
    if (emitter != NULL)
    {
        if (owner->facing == DIRECTION8_NORTH || owner->facing == DIRECTION8_NORTHWEST)
        {
            flags = emitter->field_0x2e & 0xbfff;
            bits = 0x100;
        }
        else
        {
            flags = emitter->field_0x2e & 0xfeff;
            bits = 0x4000;
        }
        emitter->field_0x2e = flags | bits;
    }
}
