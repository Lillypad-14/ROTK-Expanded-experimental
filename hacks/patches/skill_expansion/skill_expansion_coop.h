// Skill Expansion co-op messages: skill tree actions, which both consoles apply on the same frame,
// and the page 2 record of each player's slot, sent when a co-op session loads its slots.
#pragma once

#include "types.h"

// Link message id of the Skill Expansion's messages. The stock game sends none with this id.
// skill_expansion.asm has the same value.
#define SKILL_EXPANSION_MP_MSG 0x0b

// Message payload bytes after the kind byte.
#define SKILL_EXPANSION_MP_DATA_SIZE 7

/** Payload byte 0 of a SKILL_EXPANSION_MP_MSG message: what the rest of the payload holds. */
enum SkillExpansionMpKind
{
    // An unswapped active row, then TRUE to pay with the Tally.
    SKILL_EXPANSION_MP_BUY_ACTIVE = 1,
    // A PassiveSkillId, then TRUE to pay with the Tally.
    SKILL_EXPANSION_MP_BUY_PASSIVE = 2,
    // An unswapped active row to take out of the cast cycle or put back.
    SKILL_EXPANSION_MP_TOGGLE_CYCLE = 3,
    // The three unswapped quick-select rows.
    SKILL_EXPANSION_MP_QUICK_SELECT = 4,
    // A chunk index, then that chunk of the sender's page 2 record.
    SKILL_EXPANSION_MP_PEER_RECORD = 5,
};

/** Queue a message for both consoles. `data` holds SKILL_EXPANSION_MP_DATA_SIZE bytes. */
void skillExpansion_coopSend(u8 kind, const u8 *data);

void skillExpansion_coopReceive(u32 sender, const u8 *message, bool32 fromPartner);

/**
 * Apply a skill tree action to a player's skills, with page 2 not swapped in.
 *
 * @param local TRUE for this console's player
 */
void skillExpansion_treeApply(u8 playerIndex, u8 kind, const u8 *data, bool local);

/** Store a chunk of the page 2 record of `sender`, the co-op peer. */
void skillExpansion_peerRecordReceive(u8 sender, const u8 *data);
