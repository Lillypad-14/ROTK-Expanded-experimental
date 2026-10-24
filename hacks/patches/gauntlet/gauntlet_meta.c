#include "patches/gauntlet/gauntlet_meta.h"

#include "memory.h"
#include "patches/mod_save/save_sram.h"

#define GAUNTLET_META_TAG 0x47
#define EMBERS_MAX 0xffff
#define EMBERS_PER_ROOM 5
#define KILLS_PER_EMBER 5

_Static_assert(sizeof(GauntletMeta) <= sizeof(((union GlobalModOptionsArea *)0)->d.gauntletState),
               "GauntletMeta outgrew its save grant");

typedef struct GauntletUnlockDef
{
    const char *name;
    // One line of the unlocks panel.
    const char *description;
    u8 maxRank;
    // cost of rank 1, each later rank costs this times its number
    u8 baseCost;
} GauntletUnlockDef;

static const GauntletUnlockDef Unlocks[GAUNTLET_UNLOCK_COUNT] = {
    [GAUNTLET_UNLOCK_VITALITY] = {"Vitality", "+3 Health at the start of each run, per rank.", 3, 30},
    [GAUNTLET_UNLOCK_MENDING] = {"Mending", "Room clears heal 10% more HP per rank.", 3, 25},
    [GAUNTLET_UNLOCK_BLESSING] = {"Blessing", "Start with a random boon. Rank 2: a Rare one.", 2, 60},
    [GAUNTLET_UNLOCK_FATE] = {"Fate", "R rerolls a boon offer. One use per rank a run.", 3, 40},
    [GAUNTLET_UNLOCK_VETERAN] = {"Veteran", "Start every run knowing a second skill.", 1, 100},
};

/** A meta block with every field read as blank-safe. */
static void sanitize(GauntletMeta *meta)
{
    u32 i;

    if (meta->tag != GAUNTLET_META_TAG)
    {
        memory_memset(meta, 0, sizeof(*meta));
        meta->tag = GAUNTLET_META_TAG;
        return;
    }
    if (meta->lastHero >= GAUNTLET_META_HERO_COUNT)
    {
        meta->lastHero = 0;
    }
    for (i = 0; i < GAUNTLET_UNLOCK_COUNT; i++)
    {
        if (meta->unlocks[i] > Unlocks[i].maxRank)
        {
            meta->unlocks[i] = Unlocks[i].maxRank;
        }
    }
}

void gauntletMeta_load(GauntletMeta *meta)
{
    u8 area[sizeof(((union GlobalModOptionsArea *)0)->d.gauntletState)];

    GLOBAL_MOD_OPTIONS_READ(gauntletState, area);
    memory_memcpy(meta, area, sizeof(*meta));
    sanitize(meta);
}

void gauntletMeta_save(const GauntletMeta *meta)
{
    u8 area[sizeof(((union GlobalModOptionsArea *)0)->d.gauntletState)] = {0};

    memory_memcpy(area, meta, sizeof(*meta));
    GLOBAL_MOD_OPTIONS_WRITE(gauntletState, area);
}

u32 gauntletMeta_embers(const GauntletMeta *meta)
{
    return meta->embersLow | meta->embersHigh << 8;
}

static void setEmbers(GauntletMeta *meta, u32 embers)
{
    if (embers > EMBERS_MAX)
    {
        embers = EMBERS_MAX;
    }
    meta->embersLow = embers;
    meta->embersHigh = embers >> 8;
}

u32 gauntletMeta_runs(const GauntletMeta *meta)
{
    return meta->runsLow | meta->runsHigh << 8;
}

u32 gauntletMeta_rank(u32 unlock)
{
    GauntletMeta meta;

    gauntletMeta_load(&meta);
    return unlock < GAUNTLET_UNLOCK_COUNT ? meta.unlocks[unlock] : 0;
}

const char *gauntletMeta_unlockName(u32 unlock)
{
    return Unlocks[unlock].name;
}

const char *gauntletMeta_unlockDescription(u32 unlock)
{
    return Unlocks[unlock].description;
}

u32 gauntletMeta_unlockMaxRank(u32 unlock)
{
    return Unlocks[unlock].maxRank;
}

u32 gauntletMeta_unlockCost(const GauntletMeta *meta, u32 unlock)
{
    u32 rank = meta->unlocks[unlock];

    return rank >= Unlocks[unlock].maxRank ? 0 : Unlocks[unlock].baseCost * (rank + 1);
}

bool32 gauntletMeta_buy(GauntletMeta *meta, u32 unlock)
{
    u32 cost = gauntletMeta_unlockCost(meta, unlock);
    u32 embers = gauntletMeta_embers(meta);

    if (cost == 0 || cost > embers)
    {
        return FALSE;
    }
    setEmbers(meta, embers - cost);
    meta->unlocks[unlock]++;
    gauntletMeta_save(meta);
    return TRUE;
}

u32 gauntletMeta_runReward(u32 roomsCleared, u32 kills)
{
    return roomsCleared * EMBERS_PER_ROOM + kills / KILLS_PER_EMBER;
}

bool32 gauntletMeta_recordRun(u32 heroId, u32 roomsCleared, u32 kills)
{
    GauntletMeta meta;
    bool32 newBest = FALSE;
    u32 runs;

    gauntletMeta_load(&meta);
    setEmbers(&meta, gauntletMeta_embers(&meta) + gauntletMeta_runReward(roomsCleared, kills));
    runs = gauntletMeta_runs(&meta) + 1;
    if (runs <= 0xffff)
    {
        meta.runsLow = runs;
        meta.runsHigh = runs >> 8;
    }
    if (heroId < GAUNTLET_META_HERO_COUNT && roomsCleared > meta.best[heroId])
    {
        meta.best[heroId] = roomsCleared > 0xff ? 0xff : roomsCleared;
        newBest = TRUE;
    }
    gauntletMeta_save(&meta);
    return newBest;
}
