#pragma once

// Ringwraith gear: wraith-only bases, affixes and uniques appended past the stock item tables.

#include "item.h"

// Wraith-exclusive item effects, summed over the equipped wraith gear.
enum WraithGearEffect
{
    WRAITH_FX_NONE = 0,
    WRAITH_FX_DREAD_PER_HIT = 1, // +N Dread per melee hit
    WRAITH_FX_FEAR_REACH = 2,    // +N Dread aura radius
    WRAITH_FX_LIFE_DRAIN = 3,    // N% of melee damage dealt heals the wraith
    WRAITH_FX_KILL_SPIRIT = 4,   // +N Spirit per kill
    WRAITH_FX_FRIGHT_DAMAGE = 5, // +N% melee damage to frightened foes
    WRAITH_FX_TERROR_STUN = 6,   // +N frames of terror stun
    WRAITH_FX_UNSEEN = 7,        // +N frames of The Unseen World
    WRAITH_FX_DRAIN_HEAL = 8,    // +N% Black Breath and No Living Man healing
    WRAITH_FX_WARD = 9,          // N% less damage taken
    WRAITH_FX_THRALL_DAMAGE = 10, // +N thrall damage
    WRAITH_FX_THRALL_FRAMES = 11, // +N frames of thrall lifetime
    WRAITH_FX_UNSEEN_DAMAGE = 12, // +N% damage of Unseen blows
    WRAITH_FX_TERROR_SPIRIT = 13, // +N Spirit per foe terrified
    WRAITH_FX_COUNT = 14,
};

// Wraith uniques. Each one bends a kit rule while equipped.
enum WraithUnique
{
    WRAITH_UNIQUE_WITCH_KINGS_MACE = 0, // kills spread Dread around the victim
    WRAITH_UNIQUE_MORGUL_KNIFE = 1,     // melee hits embed Morgul shards for less damage
    WRAITH_UNIQUE_IRON_CROWN = 2,       // double fear reach, no Dread decay, more damage taken
    WRAITH_UNIQUE_BARROW_HAUBERK = 3,   // damage drains Spirit before health
    WRAITH_UNIQUE_NINTH_RING = 4,       // every ninth melee hit terrifies
    WRAITH_UNIQUE_SHROUD_OF_UNBEING = 5, // hits keep Unseen, shorter Unseen, 5x Unseen blows
    WRAITH_UNIQUE_BLACK_CAPTAIN = 6,    // double damage and drain against foes at full Dread
    WRAITH_UNIQUE_MORGUL_VALE = 7,      // periodic Dread on the nearest foe
    WRAITH_UNIQUE_BANNER_OF_ANGMAR = 8, // one more thrall, thralls hit harder
    WRAITH_UNIQUE_BARROW_CROWN = 9,     // foes near thralls gain Dread each pulse
    WRAITH_UNIQUE_CARN_DUM_SIGNET = 10, // Command of the Nine spares one thrall
    WRAITH_UNIQUE_BLACK_DART = 11,      // Unseen blows plant a Morgul shard
    WRAITH_UNIQUE_DEAD_MARSHES = 12,    // longer Unseen World
    WRAITH_UNIQUE_UNLIT_HAND = 13,      // Shadowstep kills refund its cost
    WRAITH_UNIQUE_MORGUL_HORN = 14,     // Cry of the Nazgul hits double and refunds half
    WRAITH_UNIQUE_DOL_GULDUR_MASK = 15, // terrified foes burst on death
    WRAITH_UNIQUE_VOICE_OF_SAURON = 16, // half aura radius, double aura Dread
    WRAITH_UNIQUE_RING_OF_KHAMUL = 17,  // every kill heals
    WRAITH_UNIQUE_FELL_BEAST_SADDLE = 18, // wider Fell Beast that terrifies
    WRAITH_UNIQUE_COUNT = 19,
};

typedef struct WraithGearStats
{
    // summed WraithGearEffect values, indexed by WraithGearEffect
    s16 effects[WRAITH_FX_COUNT];

    // equipped uniques, bit WraithUnique (WRAITH_UNIQUE_COUNT must stay <= 32)
    u32 uniques;
} WraithGearStats;

bool32 wraithGear_isWraithItem(Item item);

/** Recompute the equipped wraith gear totals. */
void wraithGear_refresh(u8 playerIndex);

const WraithGearStats *wraithGear_stats(u8 playerIndex);

static inline s32 wraithGear_effect(u8 playerIndex, u32 effect)
{
    return wraithGear_stats(playerIndex)->effects[effect];
}

static inline bool32 wraithGear_hasUnique(u8 playerIndex, u32 unique)
{
    return (wraithGear_stats(playerIndex)->uniques >> unique) & 1;
}

/** Wraith gear name for a wraith TextId, or NULL. */
const char *wraithGear_findString(s32 textId);

/** Item info text of a wraith item, in g_TextStringBuffer. */
char *wraithGear_describe(Item item);
