// Auto loot: pick up gem stacks without an R press. Hooked from actor_lootPile_tick
// (auto_loot.asm), the drop actor's per-frame update. When a player is within pickup range
// of a gem stack and "Auto-Collect Gems" is on, the game's own pickup-input handler
// (actor_lootPile_trigger) is called with the standing flag set and R faked in
// g_PlayerKeysPressed for that one call, so the stack is looted exactly as if R was pressed
// on it. Both co-op consoles run it for both players, so they loot the same stacks.
//
// The range test repeats actor_lootPile_tick's own standing-flag check (same helpers,
// same distance) instead of reading standingFlags: the game hands the flag to only
// one drop per player per frame (the first in range claims the player via a HUD
// flag), so a stack under other drops never holds it and would never auto-loot.
#include "patches/loot_qol/auto_loot.h"
#include "combat.h"
#include "gba_io.h"
#include "input.h"
#include "loot.h"
#include "patches/loot_qol/loot_qol.h"
#include "patches/mod_save/save_sram.h"
#include "variables.h"

#if HACK_mod_options
/** The mod_options row accessors (the ModOptionEntry get/set signatures). */
u8 autoLoot_getGems(void)
{
    return g_LootQolState.autoLootGems;
}

void autoLoot_setGems(u8 value)
{
    g_LootQolState.autoLootGems = value;
    GLOBAL_MOD_OPTIONS_WRITE(lootQolState, &g_LootQolState);
}
#endif

void autoLoot_onDropUpdate(Actor *drop)
{
    Actor *playerActor = NULL;
    s32 nearest = LOOT_PICKUP_DISTANCE;
    u32 looter = 0;
    u32 p;

    // Actor.variant holds the LootType (loot.h)
    if (drop->variant != LOOT_TYPE_GEM_STACK_MIN && drop->variant != LOOT_TYPE_GEM_STACK)
    {
        return;
    }
    // the picked-up state is terminal; every other state still accepts R, so mirror that
    if (drop->behaviorState == LOOT_DROP_PICKED_UP)
    {
        return;
    }
    if (!g_LootQolState.autoLootGems)
    {
        return;
    }

    // the nearest player in range takes the stack, the first one on a tie
    for (p = 0; p < g_PlayerGlobals.activePlayerCount; p++)
    {
        Actor *actor = PLAYER(p).ownerActor;
        s32 distance;

        if (actor == NULL)
        {
            continue;
        }
        distance = actor_distance(drop, actor);
        if (distance < nearest)
        {
            nearest = distance;
            playerActor = actor;
            looter = p;
        }
    }
    if (playerActor == NULL)
    {
        return;
    }

    // actor_lootPile_trigger re-checks the standing flag, so claim it for this one call;
    // actor_lootPile_tick runs right after this hook and rewrites the flag
    drop->as.lootPile.standingFlags[looter] = 1;

    // simulate the R button press and run the handler
    PLAYER_KEYS_PRESSED(looter) |= R_BUTTON;
    actor_lootPile_trigger(drop, playerActor);
    PLAYER_KEYS_PRESSED(looter) &= ~R_BUTTON;
}
