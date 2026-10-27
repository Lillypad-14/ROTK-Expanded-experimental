#include "patches/skill_expansion/skill_expansion_coop.h"
#include "memory.h"
#include "multiplayer.h"

// Message byte of the payload. multiplayer_queueMessage puts the id and sequence before it.
#define MESSAGE_PAYLOAD_OFFSET 4

void skillExpansion_coopSend(u8 kind, const u8 *data)
{
    union {
        MpMessageArgs args;
        u8 bytes[sizeof(MpMessageArgs)];
    } message;

    memory_memset(&message, 0, sizeof(message));
    message.bytes[0] = kind;
    memory_memcpy(&message.bytes[1], data, SKILL_EXPANSION_MP_DATA_SIZE);
    multiplayer_queueMessage(SKILL_EXPANSION_MP_MSG, message.args);
}

/**
 * Handle a SKILL_EXPANSION_MP_MSG message. multiplayer_dispatchMessages runs it on both consoles
 * for each player's message, the sender's own included.
 *
 * @param sender the sending player's index
 * @param fromPartner TRUE when the other console sent it
 */
void skillExpansion_coopReceive(u32 sender, const u8 *message, bool32 fromPartner)
{
    const u8 *payload = message + MESSAGE_PAYLOAD_OFFSET;

    if (payload[0] == SKILL_EXPANSION_MP_PEER_RECORD)
    {
#if HACK_mod_save
        if (fromPartner)
        {
            skillExpansion_peerRecordReceive(sender, &payload[1]);
        }
#endif
        return;
    }
    skillExpansion_treeApply(sender, payload[0], &payload[1], !fromPartner);
}
