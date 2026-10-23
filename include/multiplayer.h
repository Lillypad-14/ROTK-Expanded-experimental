#pragma once

#include "types.h"

/** Link-cable co-op message ids (payload layout varies per id). */
enum MultiplayerMsgId
{
    // content-free: the receive dispatcher only advances the message sequence
    // (sent on scene transitions and as a generic handshake tick)
    MP_MSG_SYNC = 0x01,
    MP_MSG_SEED = 0x03,            // host -> client: shared 16-bit random seed (+ unlock flags)
    MP_MSG_SAVE_SLOT_BEGIN = 0x14, // start a save-slot transfer (arg = slot index)
    MP_MSG_SAVE_SLOT_CHUNK = 0x15, // 8 bytes of save-slot data
    // save-slot receive ack: the sender re-chunks until the peer has all 0x70 bytes,
    // then sets MP_MSG_FLAG_PEER_READY
    MP_MSG_SAVE_SLOT_ACK = 0x16,
    MP_MSG_SHOP_PURCHASE = 0x23, // scene_shop_purchaseService args
    MP_MSG_SHOP_SELL = 0x24,     // scene_shop_sellItem args
    MP_MSG_TO_MAIN_MENU = 0x28,  // both peers back out to the main menu
    MP_MSG_HOLLOW_STORE = 0x2f,  // scene_shop_storeItemInHollow args
    MP_MSG_HOLLOW_TAKE = 0x30,   // scene_shop_takeItemFromHollow args
    // host -> both: open the save menu in SAVE_LOAD_MODE_COOP_LOAD
    MP_MSG_COOP_SAVE_MENU = 0x32,
};

/** g_MpMessageMask bits. The word doubles as the link session state. */
enum MpMessageFlag
{
    // save/load co-op handshake step-done bits: the peer finished (our data ack'd) /
    // our side finished (their data received)
    MP_MSG_FLAG_PEER_READY = 1 << 0,
    MP_MSG_FLAG_LOCAL_READY = 1 << 1,
    MP_MSG_FLAG_2 = 1 << 2, // map-overlay scenes pump the live world while set
    // player N has a menu overlay open (blocks pausing and mission warps)
    MP_MSG_FLAG_MENU_OPEN_PLAYER0 = 1 << 11,
    MP_MSG_FLAG_MENU_OPEN_PLAYER1 = 1 << 12,
};

/** Message payload words, passed by value; the per-word layout varies per msgId. */
typedef struct MpMessageArgs
{
    u32 words[3];
} MpMessageArgs;

/**
 * Queue a 12-byte co-op message (id + sequence + payload) for the link partner and mark it
 * pending (g_GameFlags |= 1 << 12). Shop actions route through this instead
 * of applying locally when the co-op flag (g_GameFlags & GAME_FLAG_COOP_ACTIVE) is
 * set.
 */
void multiplayer_queueMessage(u8 msgId, MpMessageArgs args);

/** Full link-subsystem reset (SIO registers, message pools). */
void multiplayer_resetLink(void);
/** Start/poll a link session for playerCount players. Returns 5 = busy, 7 = aborted/timeout. */
s32 multiplayer_startSession(u32 playerCount);
s32 multiplayer_pollSession(void);
void multiplayer_abortSession(void);
void multiplayer_syncBarrier(void);
void multiplayer_endSession(void);
s32 multiplayer_getMultiPlayerId(void);
void multiplayer_setAbortScene(u32 sceneId);
void multiplayer_clearSessionFlag(u8 clear);
/** Bitmask of received link messages. */
u32 multiplayer_getMessageMask(void);
/** Whether all expected players have connected. */
bool multiplayer_allPlayersConnected(void);

/**
 * Per-frame co-op pump (game_tickFrame head): on link loss tears the session down
 * and requests the fallback scene, otherwise queues the per-frame status message.
 */
void multiplayer_tick(void);
