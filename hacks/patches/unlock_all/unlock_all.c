#include "patches/unlock_all/unlock_all.h"
#include "save.h"
#include "sfx.h"
#include "variables.h"

#if HACK_mod_options
#define UNLOCK_ALL_BONUS_MAPS                                                                      \
    (UNLOCK_FLAG_BONUS_MAP_HELMS_DEEP | UNLOCK_FLAG_BONUS_MAP_FANGORN |                            \
     UNLOCK_FLAG_BONUS_MAP_WEATHERTOP | UNLOCK_FLAG_BONUS_MAP_MORIA)
#define UNLOCK_ALL_HERO_PATHS                                                                      \
    (UNLOCK_FLAG_FINISHED_AS_FRODO | UNLOCK_FLAG_FINISHED_AS_LEGOLAS |                             \
     UNLOCK_FLAG_FINISHED_AS_EOWYN | UNLOCK_FLAG_FINISHED_AS_ARAGORN |                             \
     UNLOCK_FLAG_FINISHED_AS_GANDALF | UNLOCK_FLAG_FINISHED_AS_GIMLI)

static bool isUnlocked(u32 mask)
{
    return (g_SaveGlobals.d.unlockFlags.p & mask) == mask;
}

/** Set the bits of mask and persist the global save header. A value of 0 is ignored. */
static void unlock(u32 mask, u8 value)
{
    if (value == 0 || isUnlocked(mask))
    {
        return;
    }
    g_SaveGlobals.d.unlockFlags.p |= mask;
    sfx_disableAudio();
    save_writeGlobalHeader();
    sfx_enableAudio();
    sfx_play(SFX_UNLOCK);
}

static u8 getBonusMaps(void)
{
    return isUnlocked(UNLOCK_ALL_BONUS_MAPS);
}

static void setBonusMaps(u8 value)
{
    unlock(UNLOCK_ALL_BONUS_MAPS, value);
}

static u8 getSam(void)
{
    return isUnlocked(UNLOCK_FLAG_CHARACTER_SAM);
}

static void setSam(u8 value)
{
    unlock(UNLOCK_FLAG_CHARACTER_SAM, value);
}

static u8 getSmeagol(void)
{
    return isUnlocked(UNLOCK_FLAG_CHARACTER_SMEAGOL);
}

static void setSmeagol(u8 value)
{
    unlock(UNLOCK_FLAG_CHARACTER_SMEAGOL, value);
}

#if HACK_chapter_select
static u8 getHeroPaths(void)
{
    return isUnlocked(UNLOCK_ALL_HERO_PATHS);
}

static void setHeroPaths(u8 value)
{
    unlock(UNLOCK_ALL_HERO_PATHS, value);
}
#endif

static const char *const UnlockAllNames[2] = {"Locked", "Unlocked"};

const struct ModOptionEntry UnlockAllOptions[UNLOCK_ALL_OPTION_COUNT] = {
    {"Bonus Maps", UnlockAllNames, 2, 0, getBonusMaps, setBonusMaps,
     "Helm's Deep, Fangorn,\nWeathertop and Moria."},
    {"Sam", UnlockAllNames, 2, 0, getSam, setSam,
     "Play as Sam from the\ncharacter select."},
    {"Smeagol", UnlockAllNames, 2, 0, getSmeagol, setSmeagol,
     "Play as Smeagol from the\ncharacter select."},
#if HACK_chapter_select
    {"Hero Paths", UnlockAllNames, 2, 0, getHeroPaths, setHeroPaths,
     "Unlock every hero path in\nChapter Select."},
#endif
};
#endif
