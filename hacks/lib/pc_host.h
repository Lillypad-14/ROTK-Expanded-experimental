#pragma once

// Info block the ROTK Recompiled PC host writes at the start of its host RAM (0x12000000). Absent on
// a GBA, where the magic never matches, so every accessor falls back to the stock sizes.

#include "actor.h"
#include "text.h"
#include "types.h"

#define PC_HOST_INFO_ADDRESS 0x12000000
#define PC_HOST_INFO_MAGIC 0x31435052 // "RPC1"

typedef struct PcHostInfo
{
    // PC_HOST_INFO_MAGIC when the PC host is present
    u32 magic;
    // layout version, 4
    u32 version;
    // slots in the main actor pool (stock 0x100)
    u32 mainActors;
    // slots in the overlay actor pool (stock 0x28)
    u32 overlayActors;
    // main actor pool address
    u32 mainPool;
    // overlay actor pool address
    u32 overlayPool;
    // game heap start
    u32 heapBase;
    // game heap end
    u32 heapEnd;
    // enemies to spawn around the player for a stress test, counts down as they spawn (version 2)
    u32 stressSpawn;
    // the Versus duel's scoreboard, written by the mod every tick for the host to draw (version 3):
    // PC_HOST_DUEL_* fields, 0 outside a duel
    u32 duel;
    // address of the TwoTowersData block, 0 when The Two Towers ROM was not found (version 4)
    u32 twoTowers;
    // address of the host RAM ItemTypeInfos copy with legacy item rows, 0 without it (version 4)
    u32 itemTypeInfos;
    // address of 4 KB of zeroed host RAM for legacy item bookkeeping, 0 without it (version 4)
    u32 legacyScratch;
} PcHostInfo;

#define PC_HOST_DUEL_PHASE(duel) ((duel) & 3)  // 1 = fighting, 2 = won
#define PC_HOST_DUEL_SCORE(duel, playerIndex) (((duel) >> (8 + 8 * (playerIndex))) & 0xff)
#define PC_HOST_DUEL_TARGET(duel) ((duel) >> 24)
#define PC_HOST_DUEL(phase, score0, score1, target)                                                    ((u32)(phase) | (u32)(score0) << 8 | (u32)(score1) << 16 | (u32)(target) << 24)

static inline const volatile PcHostInfo *hackLib_pcHostInfo(void)
{
#if defined(__arm__)
    const volatile PcHostInfo *info = (const volatile PcHostInfo *)PC_HOST_INFO_ADDRESS;
    return info->magic == PC_HOST_INFO_MAGIC ? info : NULL;
#else
    // Host unit tests have no guest address space.
    return NULL;
#endif
}

static inline volatile PcHostInfo *hackLib_pcHostInfoWritable(void)
{
    return (volatile PcHostInfo *)hackLib_pcHostInfo();
}

static inline u32 hackLib_mainActorCapacity(void)
{
    const volatile PcHostInfo *info = hackLib_pcHostInfo();
    return info != NULL ? info->mainActors : ACTOR_POOL_MAIN_CAPACITY;
}

static inline u32 hackLib_overlayActorCapacity(void)
{
    const volatile PcHostInfo *info = hackLib_pcHostInfo();
    return info != NULL ? info->overlayActors : ACTOR_POOL_OVERLAY_CAPACITY;
}

#define TWO_TOWERS_DATA_MAGIC 0x31445454 // "TTD1"

// The Two Towers data the PC host copies from The Two Towers ROM, for items imported from Two
// Towers saves. An item record is 16 bytes: u16 name text id, s16 gem value, then 5 pairs of u8
// stat and s8 value, with stat 0x40 for none.
typedef struct TwoTowersData
{
    // TWO_TOWERS_DATA_MAGIC
    u32 magic;
    // prefix records
    const u8 *prefixes;
    u32 prefixCount;
    // base item records, every item type in one table
    const u8 *baseItems;
    u32 baseItemCount;
    // suffix records
    const u8 *suffixes;
    u32 suffixCount;
    // number of Two Towers text ids
    u32 stringCount;
    // per Language, NUL-terminated font strings by Two Towers text id, NULL for one not carried
    const char *const *strings[LANGUAGE_COUNT];
    // the Two Towers save's character slots, 0x78 bytes each, zeroed for an empty one
    const u8 *saveSlots;
    u32 saveSlotCount;
    // per base item: its backpack icon in ROTK's style, a SpriteAnimation (NULL tiles for none)
    const u8 *baseItemIcons;
    // per hero, its 11 skills' icon art as ROTK skill icon tiles (32x24, 4bpp, the art in colors
    // 8..13 and the rest 0)
    const u8 *skillArt;
    // {Two Towers address, ROTK SpriteAnimation pointer} per Two Towers animation the PC host
    // converted
    const u32 *sprites;
    u32 spriteCount;
} TwoTowersData;

static inline const TwoTowersData *hackLib_twoTowersData(void)
{
    const volatile PcHostInfo *info = hackLib_pcHostInfo();
    if (info == NULL || info->version < 4 || info->twoTowers == 0)
        return NULL;
    const TwoTowersData *data = (const TwoTowersData *)info->twoTowers;
    return data->magic == TWO_TOWERS_DATA_MAGIC ? data : NULL;
}
