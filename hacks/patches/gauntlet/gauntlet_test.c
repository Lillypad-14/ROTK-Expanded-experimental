#include "patches/gauntlet/gauntlet_test.h"

#include "game.h"
#include "gba_io.h"
#include "gfx.h"
#include "lib/combat_source.h"
#include "lib/movement.h"
#include "memory.h"
#include "patches/gauntlet/gauntlet.h"
#include "patches/gauntlet/gauntlet_boons.h"
#include "patches/gauntlet/gauntlet_meta.h"
#include "patches/mod_ram/memory_diagnostics.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "player.h"
#include "rng.h"
#include "save.h"
#include "stats.h"
#include "text.h"
#include "variables.h"

#define REG_DEBUG_STRING ((volatile char *)0x04fff600)
#define REG_DEBUG_FLAGS (*(volatile u16 *)0x04fff700)
#define REG_DEBUG_ENABLE (*(volatile u16 *)0x04fff780)
#define DEBUG_STRING_SIZE 0x100
#define DEBUG_SEND_WARN 0x102

// Ticks between test kills.
#define KILL_INTERVAL 12
// Ticks the pick screen waits before each press.
#define PRESS_INTERVAL 90
// A room taking longer than this is reported as stuck.
#define ROOM_TIMEOUT_TICKS 5400

const volatile GauntletTestConfig gauntletTest_config = {0};

struct GauntletTestState
{
    u16 roomTicks;
    u8 killTicks;
    u8 pressTicks;
    // the pick screen moved its cursor once for this page
    u8 moved;
    u8 stuckReported;
    // the boon page was rerolled once
    u8 rerolled;
};

static struct GauntletTestState TestState;

bool32 gauntletTest_isActive(void)
{
    return gauntletTest_config.rooms != 0;
}

bool32 gauntletTest_probesMaps(void)
{
    return gauntletTest_isActive() && gauntletTest_config.probeMaps != 0;
}

static void debugPrint(const char *text)
{
    u32 i;

    for (i = 0; text[i] != '\0' && i < DEBUG_STRING_SIZE - 1; i++)
    {
        REG_DEBUG_STRING[i] = text[i];
    }
    REG_DEBUG_STRING[i] = '\0';
    REG_DEBUG_FLAGS = DEBUG_SEND_WARN;
}

static char *appendText(char *out, const char *text)
{
    while (*text != '\0')
    {
        *out++ = *text++;
    }
    *out = '\0';
    return out;
}

static char *appendNumber(char *out, s32 value)
{
    char digits[10];
    u32 magnitude = value < 0 ? -(u32)value : (u32)value;
    u32 count = 0;

    *out++ = ' ';
    if (value < 0)
    {
        *out++ = '-';
    }
    do
    {
        digits[count++] = '0' + magnitude % 10;
        magnitude /= 10;
    } while (magnitude != 0);
    while (count != 0)
    {
        *out++ = digits[--count];
    }
    *out = '\0';
    return out;
}

/** Heap headroom, allocation failures and the .bss guard, appended to a log line. */
static char *appendMemory(char *out)
{
    ModRamMemoryStats memory;

    modRam_getMemoryStats(&memory);
    out = appendNumber(out, memory.heapEnd - memory.peakTop);
    out = appendNumber(out, memory.failures);
    return appendNumber(out, modRam_isBssGuardIntact());
}

static void maxUnlocks(void)
{
    GauntletMeta meta;
    u32 unlock;

    gauntletMeta_load(&meta);
    for (unlock = 0; unlock < GAUNTLET_UNLOCK_COUNT; unlock++)
    {
        meta.unlocks[unlock] = gauntletMeta_unlockMaxRank(unlock);
    }
    gauntletMeta_save(&meta);
}

void gauntletTest_boot(void)
{
    REG_DEBUG_ENABLE = 0xc0de;
    memory_memcpy(&g_SaveGlobals, SaveHeaderTemplate, sizeof(g_SaveGlobals));
    g_SaveGlobals.d.language = LANGUAGE_EN;
    g_SaveGlobals.d.languageSet = 1;
    text_setLanguage(LANGUAGE_EN);
    debugPrint("GAUNTLET_BOOT");
    // The main menu seeds the gameplay stream in normal play.
    rng_reseed();
    if (gauntletTest_config.allUnlocks & 1)
    {
        maxUnlocks();
    }
    gauntlet_startRun(gauntletTest_config.hero);
    if (gauntletTest_config.allUnlocks & 2)
    {
        for (u32 boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
        {
            gauntletBoons_grant(boon);
        }
    }
}

/** Fell the first living foe through the player's damage path, so kills award XP. */
static void killOne(Actor *owner)
{
    Actor *cursor;

    for (cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
    {
        if (gauntlet_isLivingRunFoe(cursor))
        {
            // The hit can still miss, so the foe drops to 1 HP first.
            cursor->currentHp = 1;
            skillExpansion_dealDamage(owner, cursor, 0xffff, HACKLIB_DAMAGE_SECONDARY,
                                      HACKLIB_ABILITY_NONE);
            return;
        }
    }
}

void gauntletTest_tick(Actor *owner)
{
    char line[DEBUG_STRING_SIZE];

    if (!gauntletTest_isActive())
    {
        return;
    }
    if (gauntlet_roomsCleared() >= gauntletTest_config.rooms)
    {
        PLAYER(0).currentHp = 0;
        return;
    }
    PLAYER(0).currentHp = PLAYER_STAT(0, STAT_MAX_HP);
    if ((gauntletTest_config.allUnlocks & 2) && TestState.roomTicks % 90 == 0)
    {
        PLAYER(0).currentSpirit = PLAYER_STAT(0, STAT_MAX_SPIRIT);
        for (u32 row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
        {
            if (*skillExpansion_activeLevel(0, row) != 0 && row != ACTIVE_SKILL_HERBAL_HEALING)
            {
                skillExpansion_cast(0, row);
                break;
            }
        }
    }
    if (++TestState.killTicks >= KILL_INTERVAL)
    {
        TestState.killTicks = 0;
        killOne(owner);
    }
    if (++TestState.roomTicks >= ROOM_TIMEOUT_TICKS && !TestState.stuckReported)
    {
        TestState.stuckReported = TRUE;
        appendNumber(appendText(line, "GAUNTLET_STUCK"), gauntlet_roomsCleared() + 1);
        debugPrint(line);
    }
}

u16 gauntletTest_scenePress(u32 page, u32 cursor)
{
    if (!gauntletTest_isActive() || ++TestState.pressTicks < PRESS_INTERVAL)
    {
        return 0;
    }
    TestState.pressTicks = 0;
    // After the run: open the unlocks page from the hub and stay there.
    if (page == GAUNTLET_PAGE_HUB)
    {
        return cursor == 0 ? DPAD_DOWN : A_BUTTON;
    }
    if (page == GAUNTLET_PAGE_BRIEFING)
    {
        return A_BUTTON;
    }
    // Map probes take the first door. Other runs move across the doors to exercise every kind.
    if (page == GAUNTLET_PAGE_DOORS)
    {
        if (!gauntletTest_probesMaps() && cursor != gauntlet_roomsCleared() % GAUNTLET_DOOR_COUNT)
        {
            return DPAD_RIGHT;
        }
        return A_BUTTON;
    }
    if (page == GAUNTLET_PAGE_UNLOCKS)
    {
        return 0;
    }
    if (page == GAUNTLET_PAGE_BOON && gauntlet_rerollsLeft() != 0 && !TestState.rerolled)
    {
        TestState.rerolled = TRUE;
        return R_BUTTON;
    }
    TestState.rerolled = FALSE;
    // Move once per page before taking, so the cursor and later slots get exercised.
    if (!TestState.moved && cursor == 0 && gauntlet_roomsCleared() % 2 == 1)
    {
        TestState.moved = TRUE;
        return DPAD_DOWN;
    }
    TestState.moved = FALSE;
    return A_BUTTON;
}

void gauntletTest_roomEntered(u32 roomType, u32 map, u32 candidate, Actor *owner)
{
    char line[DEBUG_STRING_SIZE];
    char *out;
    if (!gauntletTest_isActive())
    {
        return;
    }
    out = appendText(line, "GAUNTLET_ENTRY");
    out = appendNumber(out, roomType);
    out = appendNumber(out, map);
    out = appendNumber(out, candidate);
    out = appendNumber(out, owner->coords.x >> 16);
    out = appendNumber(out, owner->coords.y >> 16);
    appendNumber(out, hackLib_canLandAt(owner, &owner->coords));
    debugPrint(line);
    out = appendText(line, "GAUNTLET_VARIANT");
    out = appendNumber(out, gauntlet_roomDoor().modifier);
    appendNumber(out, gauntlet_rewardRarity());
    debugPrint(line);
}

void gauntletTest_roomCleared(u32 room, u32 mission, u32 spawned, u32 landFails, u32 purged)
{
    char line[DEBUG_STRING_SIZE];
    char *out;

    if (!gauntletTest_isActive())
    {
        return;
    }
    out = appendText(line, "GAUNTLET_ROOM");
    out = appendNumber(out, room);
    out = appendNumber(out, mission);
    out = appendNumber(out, spawned);
    out = appendNumber(out, landFails);
    out = appendNumber(out, purged);
    out = appendNumber(out, TestState.roomTicks);
    out = appendNumber(out, PLAYER(0).level);
    out = appendNumber(out, PLAYER_STAT(0, STAT_MAX_HP));
    appendMemory(out);
    debugPrint(line);
    out = appendText(line, "GAUNTLET_BACKDROP");
    out = appendNumber(out, mission);
    out = appendNumber(out, g_TransitionState.effect);
    out = appendNumber(out, g_Bg.layers[1].control);
    appendNumber(out, g_TransitionState.field_0x09[3]);
    debugPrint(line);
    TestState.roomTicks = 0;
    TestState.stuckReported = FALSE;
}

void gauntletTest_runOver(u32 room, u32 kills)
{
    char line[DEBUG_STRING_SIZE];
    char *out;

    if (!gauntletTest_isActive())
    {
        return;
    }
    out = appendText(line, "GAUNTLET_OVER");
    out = appendNumber(out, room);
    out = appendNumber(out, kills);
    out = appendNumber(out, gauntletBoons_ownedCount());
    out = appendNumber(out, gauntlet_rerollsLeft());
    appendMemory(out);
    debugPrint(line);
    out = appendText(line, "GAUNTLET_HERBS");
    out = appendNumber(out, gauntlet_hasHerbs());
    appendNumber(out, PLAYER(0).kingsfoilHerbs.fresh);
    debugPrint(line);
}

void gauntletTest_finished(void)
{
    if (gauntletTest_isActive())
    {
        debugPrint("GAUNTLET_DONE");
    }
}
