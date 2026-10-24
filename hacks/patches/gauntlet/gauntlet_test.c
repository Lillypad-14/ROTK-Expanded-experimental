#include "patches/gauntlet/gauntlet_test.h"

#include "game.h"
#include "gba_io.h"
#include "lib/combat_source.h"
#include "lib/movement.h"
#include "memory.h"
#include "patches/gauntlet/gauntlet.h"
#include "patches/gauntlet/gauntlet_boons.h"
#include "patches/gauntlet/gauntlet_meta.h"
#include "patches/mod_ram/memory_diagnostics.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
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
    if (gauntletTest_config.allUnlocks)
    {
        maxUnlocks();
    }
    gauntlet_startRun(gauntletTest_config.hero);
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

static void followRoute(Actor *owner)
{
    Vector2Fp16 target;
    Vector2Fp16 next = owner->coords;
    s32 dx;
    s32 dy;
    if (!gauntlet_travelTarget(&target))
    {
        return;
    }
    dx = target.x - next.x;
    dy = target.y - next.y;
    next.x += dx > 4 * FP16_16_ONE    ? 4 * FP16_16_ONE
              : dx < -4 * FP16_16_ONE ? -4 * FP16_16_ONE
                                      : dx;
    next.y += dy > 4 * FP16_16_ONE    ? 4 * FP16_16_ONE
              : dy < -4 * FP16_16_ONE ? -4 * FP16_16_ONE
                                      : dy;
    if (!hackLib_canLandAt(owner, &next))
    {
        Vector2Fp16 horizontal = {next.x, owner->coords.y};
        Vector2Fp16 vertical = {owner->coords.x, next.y};
        next = hackLib_canLandAt(owner, &horizontal) ? horizontal : vertical;
    }
    if (hackLib_canLandAt(owner, &next))
    {
        owner->renderCoords.x += next.x - owner->coords.x;
        owner->renderCoords.y += next.y - owner->coords.y;
        owner->coords = next;
    }
}

static void reportRoute(Actor *owner)
{
    char line[DEBUG_STRING_SIZE];
    Vector2Fp16 target = {0};
    bool32 traveling = gauntlet_travelTarget(&target);
    char *out = appendText(line, "GAUNTLET_NAV");
    out = appendNumber(out, PLAYER(0).heroId);
    out = appendNumber(out, owner->coords.x >> 16);
    out = appendNumber(out, owner->coords.y >> 16);
    out = appendNumber(out, target.x >> 16);
    out = appendNumber(out, target.y >> 16);
    out = appendNumber(out, owner->movementCollisionBox.xMin);
    out = appendNumber(out, owner->movementCollisionBox.xMax);
    out = appendNumber(out, owner->movementCollisionBox.yMin);
    out = appendNumber(out, owner->movementCollisionBox.yMax);
    appendNumber(out, traveling);
    debugPrint(line);
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
    // Stand still first: encounters must not start just because time passed.
    if (TestState.roomTicks < 180)
    {
        Actor *cursor;
        for (cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
        {
            if (gauntlet_isLivingRunFoe(cursor) && !TestState.stuckReported)
            {
                appendNumber(appendText(line, "GAUNTLET_STUCK"), gauntlet_roomsCleared() + 1);
                debugPrint(line);
                reportRoute(owner);
                TestState.stuckReported = TRUE;
            }
        }
    }
    else
    {
        followRoute(owner);
    }
    if (++TestState.killTicks >= KILL_INTERVAL)
    {
        TestState.killTicks = 0;
        killOne(owner);
    }
    if (++TestState.roomTicks >= ROOM_TIMEOUT_TICKS && !TestState.stuckReported)
    {
        TestState.stuckReported = TRUE;
        reportRoute(owner);
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
}

void gauntletTest_finished(void)
{
    if (gauntletTest_isActive())
    {
        debugPrint("GAUNTLET_DONE");
    }
}
