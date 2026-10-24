#include "patches/gauntlet/gauntlet.h"

#include "actor/enemySpawner.h"
#include "game.h"
#include "item.h"
#include "lib/mission_scripts.h"
#include "lib/movement.h"
#include "math.h"
#include "memory.h"
#include "mission.h"
#include "mod_scenes.h"
#include "patches/gauntlet/gauntlet_boons.h"
#include "patches/gauntlet/gauntlet_level.h"
#include "patches/gauntlet/gauntlet_meta.h"
#include "patches/gauntlet/gauntlet_routes.h"
#include "patches/gauntlet/gauntlet_test.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "player.h"
#include "rng.h"
#include "scene.h"
#include "scene/gameplay.h"
#include "scene/mainMenu.h"
#include "sfx.h"
#include "stats.h"
#include "terrain.h"
#include "variables.h"

#if !HACK_skill_expansion
#error "gauntlet requires skill_expansion for its player tick"
#endif

#define WAVES_PER_ROOM GAUNTLET_SECTOR_COUNT
#define ROOM_START_DELAY 120
#define ROOM_SETTLE_DELAY 30
#define SECTOR_WARNING_DELAY 30
#define SECTOR_APPROACH_PIXELS 160
#define SECTOR_ENTRY_PATH_PIXELS 160
#define SECTOR_ENTRY_RADIUS 64
#define GUIDE_RECHECK_TICKS 8
#define GUIDE_LOOKAHEAD_PIXELS 64
#define ROOM_CLEAR_DELAY 75
#define SPAWN_INTERVAL 20
#define MAX_WAVE_FOES 12
#define BASE_ALIVE_CAP 5
#define MAX_ALIVE_CAP 8
#define ROOM_CLEAR_HEAL_PERCENT 30
#define MENDING_HEAL_PERCENT 10
#define VITALITY_HEALTH 3
#define TIER_COUNT 4
// Rooms per foe tier step.
#define ROOMS_PER_TIER 3
// Percent of spawns drawn from the room's top tier, the rest from the tiers below.
#define TOP_TIER_PERCENT 60
// XP scale during a run, roughly a level per room or two.
#define RUN_XP_PERCENT 400
// Spawned foes carry this record group and index, which no map record uses.
#define RUN_FOE_RECORD 0xff

/** Gauntlet.phase values. */
enum GauntletPhase
{
    PHASE_COMBAT = 0,
    PHASE_TRAVEL = 1,
    PHASE_SETTLING = 2,
    // the last wave is down, waiting to leave for the pick screen
    PHASE_CLEARED = 3,
    // leaving the map for the pick or results screen
    PHASE_LEAVING = 4,
};

/** Foe families. A room's map decides which one it fights. */
enum GauntletTheme
{
    THEME_GOBLINS,
    THEME_ORCS,
    THEME_URUKS,
    THEME_SPIDERS,
    THEME_MEN,
};

typedef struct GauntletRoomMap
{
    // enum MissionId
    u8 mission;
    // enum GauntletTheme
    u8 theme;
    // the first room this map can appear in
    u8 firstRoom;
    const char *name;
} GauntletRoomMap;

typedef struct GauntletFoe
{
    // EnemySpawnEntries index
    u8 spawnId;
    // enum GauntletTheme
    u8 theme;
    // 1 (weakest) to TIER_COUNT, from the stock HP and damage
    u8 tier;
} GauntletFoe;

static const GauntletRoomMap RoomMaps[] = {
    {MISSION_ISENGARD_CAMPSITE, THEME_URUKS, 1, "Isengard"},
    {MISSION_ISENGARD_FLOODED_FIELD, THEME_URUKS, 1, "Flooded Field"},
    {MISSION_FANGORN_CLEARING, THEME_URUKS, 1, "Fangorn"},
    {MISSION_HELMS_DEEP_VALLEY, THEME_URUKS, 1, "Helm's Deep"},
    {MISSION_MORIA_HALL, THEME_GOBLINS, 1, "Moria"},
    {MISSION_MORIA_TREASURE_HOARDS, THEME_GOBLINS, 1, "Treasure Hoard"},
    {MISSION_MORIA_BALINS_TOMB, THEME_GOBLINS, 1, "Balin's Tomb"},
    {MISSION_WHITE_MOUNTAINS_ICE_CAVES, THEME_GOBLINS, 1, "Ice Caves"},
    {MISSION_GORGOROTH_PLAINS, THEME_ORCS, 1, "Gorgoroth"},
    {MISSION_GORGOROTH_HILLS, THEME_ORCS, 1, "Gorgoroth Hills"},
    {MISSION_MORGUL_VALLEY, THEME_ORCS, 1, "Morgul Vale"},
    {MISSION_ITHILIEN_NORTH, THEME_ORCS, 1, "Ithilien"},
    {MISSION_CIRITH_UNGOL_PASS, THEME_SPIDERS, 1, "Cirith Ungol"},
    {MISSION_CIRITH_UNGOL_STAIRS, THEME_SPIDERS, 1, "Ungol Stairs"},
    {MISSION_ANDUIN_BANKS, THEME_MEN, 4, "Anduin"},
    {MISSION_ANDUIN_DOCKS, THEME_MEN, 4, "Anduin Docks"},
    {MISSION_DUNHARROW_HILLS, THEME_MEN, 4, "Dunharrow"},
    {MISSION_MINAS_TIRITH_BATTLEMENTS_GATES, THEME_MEN, 4, "Minas Tirith"},
};

// Tiers follow the stock max HP x attack damage of each spawn id.
static const GauntletFoe Foes[] = {
    {6, THEME_GOBLINS, 1},  {7, THEME_GOBLINS, 1},  {9, THEME_GOBLINS, 1},  {10, THEME_GOBLINS, 3},
    {8, THEME_GOBLINS, 3},  {16, THEME_ORCS, 1},    {19, THEME_ORCS, 2},    {22, THEME_ORCS, 2},
    {21, THEME_ORCS, 2},    {27, THEME_ORCS, 3},    {23, THEME_ORCS, 3},    {17, THEME_ORCS, 3},
    {25, THEME_ORCS, 3},    {20, THEME_ORCS, 3},    {42, THEME_ORCS, 3},    {29, THEME_ORCS, 4},
    {18, THEME_ORCS, 4},    {24, THEME_ORCS, 4},    {30, THEME_ORCS, 4},    {26, THEME_ORCS, 4},
    {28, THEME_ORCS, 4},    {43, THEME_ORCS, 4},    {38, THEME_URUKS, 1},   {36, THEME_URUKS, 1},
    {40, THEME_URUKS, 2},   {39, THEME_URUKS, 2},   {41, THEME_URUKS, 3},   {37, THEME_URUKS, 3},
    {35, THEME_SPIDERS, 1}, {32, THEME_SPIDERS, 1}, {34, THEME_SPIDERS, 2}, {33, THEME_SPIDERS, 3},
    {11, THEME_MEN, 2},     {13, THEME_MEN, 3},     {0, THEME_MEN, 3},      {3, THEME_MEN, 3},
    {2, THEME_MEN, 3},      {12, THEME_MEN, 4},     {14, THEME_MEN, 4},     {1, THEME_MEN, 4},
    {4, THEME_MEN, 4},
};

enum EncounterRole
{
    ENCOUNTER_PATROL,
    ENCOUNTER_AMBUSH,
    ENCOUNTER_ELITE_GUARD,
};

static const char *const EncounterNames[] = {"Patrol", "Ambush", "Elite guard"};

struct Gauntlet
{
    // enum GauntletMode
    u8 mode;
    // 1 for the first room
    u8 room;
    // 0 before the first wave of a room, then 1..WAVES_PER_ROOM
    u8 wave;
    // foes the current wave still has to spawn
    u8 toSpawn;
    // ticks until the next spawn, the first wave, or leaving a cleared room
    u16 timer;
    // foes spawned in this run, picks the next spawn spot
    u16 spawned;
    u16 kills;
    // enum GauntletPhase
    u8 phase;
    // RoomMaps index of the current room
    u8 map;
    // the hero's level when last checked
    u8 lastLevel;
    // level-ups still to pick a reward for at the next room's end
    u8 levelPicks;
    // HeroId of the run
    u8 hero;
    // boon rerolls left in the run
    u8 rerolls;
    // Mending unlock rank
    u8 mending;
    // spawns this room that found no free ground
    u8 landFails;
    // map enemies, spawners and warps removed this room
    u8 purged;
    u8 mapReady;
    u8 entering;
    u16 waypoint;
    u8 guideTimer;
    u8 roles[WAVES_PER_ROOM];
    u8 sectorSpawned;
    u8 lastSpawnId;
};

static struct Gauntlet Gauntlet;
// Shared clearance keeps approach triggers consistent across hero animation bounds.
static const Actor RouteFootprint = {.movementCollisionBox = {-16, 16, -12, 9}};

_Static_assert(ARRAY_COUNT(RoomMaps) == GAUNTLET_ROUTE_MAP_COUNT, "each map needs floor routes");

static Vector2Fp16 GuideFallback;
static bool32 GuideFallbackActive;

bool32 gauntlet_isRunning(void)
{
    return Gauntlet.mode == GAUNTLET_MODE_RUNNING;
}

s32 gauntlet_xpPercent(void)
{
    return gauntlet_isRunning() ? RUN_XP_PERCENT : 100;
}

void gauntlet_noteKill(void)
{
    Gauntlet.kills++;
}

u32 gauntlet_roomsCleared(void)
{
    return Gauntlet.room - 1;
}

u32 gauntlet_kills(void)
{
    return Gauntlet.kills;
}

/**
 * Main menu A press. The stock dispatch, except that the GCN Link row opens the Gauntlet hub.
 *
 * REPLACES (whole function): scene_mainMenu_onConfirm.
 */
void gauntlet_mainMenuOnConfirm(void)
{
    if (sfx_isPlaying(g_MainMenuSfxHandle))
    {
        sfx_stop(g_MainMenuSfxHandle);
    }
    Gauntlet.mode = GAUNTLET_MODE_OFF;
    hackLib_suppressMissionScripts = FALSE;
    switch (SCENE_ARG_MAIN_MENU_CURSOR)
    {
    case MAIN_MENU_ENTRY_NEW_GAME:
        game_requestSceneChange(SCENE_ID_NEW_GAME);
        g_MainMenuSfxHandle = sfx_play(SFX_MENU_CONFIRM);
        break;
    case MAIN_MENU_ENTRY_LOAD_GAME:
        game_requestSceneChangeWithArgs(SCENE_ID_SAVE_LOAD_MENU, 0, 0);
        g_MainMenuSfxHandle = sfx_play(SFX_MENU_CONFIRM);
        break;
    case MAIN_MENU_ENTRY_MULTIPLAYER:
        g_MainMenuSfxHandle = sfx_play(SFX_MENU_CONFIRM);
        scene_mainMenu_showLinkPrompt();
        break;
    case MAIN_MENU_ENTRY_OPTIONS:
        game_requestSceneChange(SCENE_ID_OPTIONS_MENU);
        g_MainMenuSfxHandle = sfx_play(SFX_MENU_CONFIRM);
        break;
    case MAIN_MENU_ENTRY_GCN_LINK:
        g_MainMenuSfxHandle = sfx_play(SFX_MENU_CONFIRM);
        game_requestSceneChangeWithArgs(MOD_SCENE_ID(GAUNTLET), GAUNTLET_PAGE_HUB, 0);
        break;
    case MAIN_MENU_ENTRY_BONUS_MAPS:
        game_requestSceneChangeWithArgs(SCENE_ID_SAVE_LOAD_MENU, 4, 0);
        g_MainMenuSfxHandle = sfx_play(SFX_MENU_CONFIRM);
        break;
    default:
        break;
    }
    rng_reseed();
}

/** Give the hero only its first castable skills, at level 1. */
static void grantStartingSkills(u32 count)
{
    u32 row;

    for (row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT && count != 0; row++)
    {
        if (PLAYER_ACTIVE_SKILL(0, row).callback != NULL)
        {
            *skillExpansion_activePurchased(0, row) = 1;
            *skillExpansion_activeLevel(0, row) = 1;
            count--;
        }
    }
}

/** The permanent unlocks bought in the hub. */
static void applyUnlocks(void)
{
    GauntletMeta meta;
    u32 blessing;

    gauntletMeta_load(&meta);
    PLAYER(0).baseStats.health += VITALITY_HEALTH * meta.unlocks[GAUNTLET_UNLOCK_VITALITY];
    gauntletLevel_refreshStats(0);
    PLAYER(0).currentHp = PLAYER_STAT(0, STAT_MAX_HP);
    grantStartingSkills(1 + meta.unlocks[GAUNTLET_UNLOCK_VETERAN]);
    blessing = meta.unlocks[GAUNTLET_UNLOCK_BLESSING];
    if (blessing != 0)
    {
        gauntletBoons_grantRandom(blessing == 1 ? RARITY_COMMON : RARITY_RARE);
    }
    Gauntlet.rerolls = meta.unlocks[GAUNTLET_UNLOCK_FATE];
    Gauntlet.mending = meta.unlocks[GAUNTLET_UNLOCK_MENDING];
}

/** A map for the current room: unlocked by now, and not the map just played. */
static u32 pickMap(void)
{
    u32 tries;
    u32 map;

    if (gauntletTest_probesMaps())
    {
        u32 routed = 0;
        for (map = 0; map < ARRAY_COUNT(RoomMaps); map++)
        {
            routed += GauntletRoutes[map][0].count != 0;
        }
        u32 pick = (Gauntlet.room - 1) % routed;
        for (map = 0; map < ARRAY_COUNT(RoomMaps); map++)
        {
            if (GauntletRoutes[map][0].count != 0 && pick-- == 0)
            {
                return map;
            }
        }
    }
    for (tries = 0; tries < 32; tries++)
    {
        map = gauntlet_randBelow(ARRAY_COUNT(RoomMaps));
        if (GauntletRoutes[map][0].count != 0 && RoomMaps[map].firstRoom <= Gauntlet.room &&
            (map != Gauntlet.map || Gauntlet.room == 1))
        {
            return map;
        }
    }
    return 0;
}

void gauntlet_prepareNextRoom(void)
{
    if (!Gauntlet.mapReady)
    {
        Gauntlet.map = pickMap();
        Gauntlet.mapReady = TRUE;
    }
}

const char *gauntlet_roomName(void)
{
    return RoomMaps[Gauntlet.map].name;
}

const char *gauntlet_encounterName(void)
{
    static const char *const Names[] = {"Goblins", "Orcs", "Uruk-hai", "Spiders",
                                        "Men of the East"};
    return Names[RoomMaps[Gauntlet.map].theme];
}

/** Load the current room's map. HP carries over from the last room unless healed. */
static void loadRoom(u32 gameplayMode)
{
    gauntlet_prepareNextRoom();
    Gauntlet.mapReady = FALSE;
    for (u32 i = 0; i < WAVES_PER_ROOM; i++)
    {
        Gauntlet.roles[i] = i;
    }
    for (u32 i = WAVES_PER_ROOM - 1; i > 0; i--)
    {
        u32 j = gauntlet_randBelow(i + 1);
        u8 role = Gauntlet.roles[i];
        Gauntlet.roles[i] = Gauntlet.roles[j];
        Gauntlet.roles[j] = role;
    }
    Gauntlet.waypoint = 0;
    Gauntlet.guideTimer = 0;
    GuideFallbackActive = FALSE;
    Gauntlet.entering = TRUE;
    Gauntlet.phase = PHASE_TRAVEL;
    Gauntlet.wave = 0;
    Gauntlet.toSpawn = 0;
    Gauntlet.timer = ROOM_START_DELAY;
    Gauntlet.landFails = 0;
    Gauntlet.purged = 0;
    g_MissionVariant = 0;
    g_MissionSpawnPoint[0] = g_MissionSpawnPoint[1] = GauntletRouteEntrances[Gauntlet.map];
    gauntletBoons_roomStart();
    game_requestSceneChangeWithArgs(SCENE_ID_GAMEPLAY, gameplayMode,
                                    RoomMaps[Gauntlet.map].mission);
}

void gauntlet_startRun(HeroId heroId)
{
    u32 i;

    memory_memset(&g_PlayerGlobals, 0, sizeof(g_PlayerGlobals));
    g_PlayerGlobals.activePlayerCount = 1;
    PLAYER(0).heroId = heroId;
    for (i = 0; i < ARRAY_COUNT(PLAYER(0).inventory.array); i++)
    {
        item_clearSlot(&PLAYER(0).inventory.array[i].word);
    }
    player_initHeroStats(heroId, 0);
    player_initHeroLoadout(heroId, 0);
    player_initItemsAndHollow(0);
    player_applyOptionFlags(0);
    g_GameFlags.p = GAME_FLAG_BONUS_MAP_ACTIVE;
    Gauntlet = (struct Gauntlet){.mode = GAUNTLET_MODE_RUNNING, .room = 1, .hero = heroId};
    Gauntlet.lastLevel = PLAYER(0).level;
    gauntletBoons_reset();
    applyUnlocks();
    hackLib_suppressMissionScripts = TRUE;
    sfx_play(1);
    gauntlet_prepareNextRoom();
    if (g_SceneCurrent.id != MOD_SCENE_ID(GAUNTLET))
    {
        game_requestSceneChangeWithArgs(MOD_SCENE_ID(GAUNTLET), GAUNTLET_PAGE_BRIEFING, 0);
    }
}

u32 gauntlet_hero(void)
{
    return Gauntlet.hero;
}

u32 gauntlet_rerollsLeft(void)
{
    return Gauntlet.rerolls;
}

bool32 gauntlet_useReroll(void)
{
    if (Gauntlet.rerolls == 0)
    {
        return FALSE;
    }
    Gauntlet.rerolls--;
    return TRUE;
}

bool32 gauntlet_takeLevelPick(void)
{
    if (Gauntlet.levelPicks == 0)
    {
        return FALSE;
    }
    Gauntlet.levelPicks--;
    return TRUE;
}

void gauntlet_enterNextRoom(void)
{
    loadRoom(Gauntlet.room == 1 ? GAMEPLAY_MODE_START_HEALED : GAMEPLAY_MODE_START);
}

void gauntlet_finishRun(void)
{
    gauntletTest_finished();
    Gauntlet.mode = GAUNTLET_MODE_OFF;
    hackLib_suppressMissionScripts = FALSE;
    g_GameFlags.p = 0;
}

static bool32 isEnemyType(u32 type)
{
    return (type >= ACTOR_TYPE_ENEMY && type <= ACTOR_TYPE_ENEMY + 9) || type == 48;
}

static bool32 isRunFoe(const Actor *actor)
{
    return actor->recordGroup == RUN_FOE_RECORD && actor->recordIndex == RUN_FOE_RECORD;
}

bool32 gauntlet_isLivingRunFoe(Actor *actor)
{
    return isEnemyType(actor->type) && isRunFoe(actor) && skillExpansion_isFoe(actor) &&
           !(actor->flags.p & ACTOR_FLAG_PENDING_REMOVE);
}

/**
 * Count the living foes this run spawned, and remove what the map placed itself: its enemies,
 * enemy spawners and mission warps.
 */
static u32 sweepActors(void)
{
    u32 count = 0;
    Actor *cursor;

    for (cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
    {
        u32 type = cursor->type;

        if (cursor->flags.p & ACTOR_FLAG_PENDING_REMOVE)
        {
            continue;
        }
        if ((isEnemyType(type) && !isRunFoe(cursor)) || type == ACTOR_TYPE_ENEMY_SPAWNER ||
            type == ACTOR_TYPE_ENEMY_SPAWNER_RNG || type == ACTOR_TYPE_MISSION_WARP)
        {
            cursor->flags.p |= ACTOR_FLAG_PENDING_REMOVE;
            Gauntlet.purged++;
            continue;
        }
        count += isEnemyType(type) && skillExpansion_isFoe(cursor);
    }
    return count;
}

static u32 tierCap(void)
{
    u32 tier = 1 + (Gauntlet.room - 1) / ROOMS_PER_TIER;

    return tier > TIER_COUNT ? TIER_COUNT : tier;
}

/** A random foe of the room's theme, mostly from the highest tier the room allows. */
static u8 pickSpawnId(bool32 leader)
{
    u32 theme = RoomMaps[Gauntlet.map].theme;
    u32 cap = tierCap();
    if (leader && cap < TIER_COUNT)
    {
        cap++;
    }
    u32 top = 0;
    u32 count = 0;
    u32 pick;
    u32 i;

    for (i = 0; i < ARRAY_COUNT(Foes); i++)
    {
        if (Foes[i].theme == theme && Foes[i].tier <= cap && Foes[i].tier > top)
        {
            top = Foes[i].tier;
        }
    }
    if (!leader && gauntlet_randBelow(100) >= TOP_TIER_PERCENT)
    {
        top = 0;
    }
    for (i = 0; i < ARRAY_COUNT(Foes); i++)
    {
        count += Foes[i].theme == theme && Foes[i].tier <= cap && (top == 0 || Foes[i].tier == top);
    }
    if (!leader && count > 1)
    {
        u32 alternatives = 0;
        for (i = 0; i < ARRAY_COUNT(Foes); i++)
        {
            alternatives += Foes[i].theme == theme && Foes[i].tier <= cap &&
                            (top == 0 || Foes[i].tier == top) &&
                            Foes[i].spawnId != Gauntlet.lastSpawnId;
        }
        if (alternatives != 0)
        {
            count = alternatives;
        }
        else
        {
            Gauntlet.lastSpawnId = 0xff;
        }
    }
    else
    {
        Gauntlet.lastSpawnId = 0xff;
    }
    pick = gauntlet_randBelow(count);
    for (i = 0; i < ARRAY_COUNT(Foes); i++)
    {
        if (Foes[i].theme == theme && Foes[i].tier <= cap && (top == 0 || Foes[i].tier == top) &&
            Foes[i].spawnId != Gauntlet.lastSpawnId && pick-- == 0)
        {
            return Foes[i].spawnId;
        }
    }
    return Foes[0].spawnId;
}

/** Spawn defenders at a fixed sector clearing, independent of the player's position. */
static void spawnFoe(Actor *owner)
{
    static const s8 Spots[][2] = {
        {0, 0}, {48, 0}, {-48, 0}, {0, 48}, {0, -48}, {32, 32}, {-32, -32}, {32, -32}, {-32, 32},
    };
    const GauntletRoute *route = &GauntletRoutes[Gauntlet.map][Gauntlet.wave - 1];
    const GauntletRoutePoint *anchor = &route->points[route->count - 1];
    u32 spot = Gauntlet.spawned++ % ARRAY_COUNT(Spots);
    Actor spawner = {0};
    Vector2Fp16 landing;
    Actor *foe;
    u32 tries;

    spawner.coords = (Vector2Fp16){anchor->x * FP16_16_ONE, anchor->y * FP16_16_ONE};
    bool32 leader =
        Gauntlet.roles[Gauntlet.wave - 1] == ENCOUNTER_ELITE_GUARD && Gauntlet.sectorSpawned == 0;
    u8 spawnId = pickSpawnId(leader);
    foe = actor_enemySpawner_spawnChild(spawnId, &spawner);
    if (foe == NULL)
    {
        return;
    }
    foe->parentActor = NULL;
    foe->recordGroup = foe->recordIndex = RUN_FOE_RECORD;
    for (tries = 0; tries < ARRAY_COUNT(Spots); tries++)
    {
        const s8 *offset = Spots[(spot + tries) % ARRAY_COUNT(Spots)];
        Vector2Fp16 preferred = {(anchor->x + offset[0]) * FP16_16_ONE,
                                 (anchor->y + offset[1]) * FP16_16_ONE};

        if (hackLib_findLanding(foe, &preferred, &landing) &&
            (ABS((landing.x - owner->coords.x) >> 16) >= 32 ||
             ABS((landing.y - owner->coords.y) >> 16) >= 32))
        {
            foe->renderCoords.x += landing.x - foe->coords.x;
            foe->renderCoords.y += landing.y - foe->coords.y;
            foe->coords = landing;
            if (leader)
            {
                s32 hp = foe->currentHp + foe->currentHp / 2;
                foe->currentHp = hp > 32767 ? 32767 : hp;
            }
            Gauntlet.sectorSpawned++;
            Gauntlet.lastSpawnId = spawnId;
            return;
        }
    }
    foe->flags.p |= ACTOR_FLAG_PENDING_REMOVE;
    Gauntlet.landFails++;
}

static u32 waveSize(void)
{
    u32 role = Gauntlet.roles[Gauntlet.wave - 1];
    u32 size;
    if (role == ENCOUNTER_PATROL)
    {
        size = 3 + Gauntlet.room / 6;
        return size > 5 ? 5 : size;
    }
    if (role == ENCOUNTER_ELITE_GUARD)
    {
        size = 3 + Gauntlet.room / 5;
        return size > 6 ? 6 : size;
    }
    size = 4 + Gauntlet.room / 2;
    return size > MAX_WAVE_FOES ? MAX_WAVE_FOES : size;
}

static u32 aliveCap(void)
{
    u32 cap = BASE_ALIVE_CAP + Gauntlet.room / 4;

    return cap > MAX_ALIVE_CAP ? MAX_ALIVE_CAP : cap;
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

static char *appendNumber(char *out, u32 value)
{
    char digits[10];
    u32 count = 0;

    do
    {
        digits[count++] = '0' + value % 10;
        value /= 10;
    } while (value != 0);
    while (count != 0)
    {
        *out++ = digits[--count];
    }
    *out = '\0';
    return out;
}

/** The HUD line's own buffer, which skillExpansion_feedback redraws whenever it is passed. */
static char *hudLine(void)
{
    return g_PlayerHuds[0].textBuffer;
}

static void announceRoom(void)
{
    char *out = appendText(hudLine(), "Room ");

    out = appendNumber(out, Gauntlet.room);
    out = appendText(out, ": ");
    out = appendText(out, RoomMaps[Gauntlet.map].name);
    out = appendText(out, " - ");
    appendText(out, gauntlet_encounterName());
    skillExpansion_feedback(0, hudLine());
}

static void announceWave(void)
{
    char *out = appendText(hudLine(), "Sector ");

    out = appendNumber(out, Gauntlet.wave);
    out = appendText(out, "/");
    out = appendNumber(out, WAVES_PER_ROOM);
    out = appendText(out, ": ");
    appendText(out, EncounterNames[Gauntlet.roles[Gauntlet.wave - 1]]);
    skillExpansion_feedback(0, hudLine());
}

static void clearRoom(void)
{
    u32 maxHp = PLAYER_STAT(0, STAT_MAX_HP);
    u32 hp = PLAYER(0).currentHp +
             maxHp * (ROOM_CLEAR_HEAL_PERCENT + MENDING_HEAL_PERCENT * Gauntlet.mending) / 100;

    PLAYER(0).currentHp = hp > maxHp ? maxHp : hp;
    Gauntlet.phase = PHASE_LEAVING;
    gauntletTest_roomCleared(Gauntlet.room, RoomMaps[Gauntlet.map].mission, Gauntlet.spawned,
                             Gauntlet.landFails, Gauntlet.purged);
    Gauntlet.room++;
    game_requestSceneChangeWithArgs(MOD_SCENE_ID(GAUNTLET), GAUNTLET_PAGE_BOON, 0);
}

static void fall(void)
{
    Gauntlet.phase = PHASE_LEAVING;
    gauntletTest_runOver(Gauntlet.room, Gauntlet.kills);
    game_requestSceneChangeWithArgs(MOD_SCENE_ID(GAUNTLET), GAUNTLET_PAGE_RESULTS, 0);
}

bool32 gauntlet_travelTarget(Vector2Fp16 *position)
{
    const GauntletRoute *route;
    const GauntletRoutePoint *point;
    if (Gauntlet.phase != PHASE_TRAVEL || Gauntlet.wave >= WAVES_PER_ROOM)
    {
        return FALSE;
    }
    if (GuideFallbackActive)
    {
        *position = GuideFallback;
        return TRUE;
    }
    route = &GauntletRoutes[Gauntlet.map][Gauntlet.wave];
    point = &route->points[Gauntlet.waypoint];
    *position = (Vector2Fp16){point->x * FP16_16_ONE, point->y * FP16_16_ONE};
    return TRUE;
}

// Retain a foe identity only while it remains in the live actor list.
static Actor *CompassFoe;
static u8 CompassFrame;

u32 gauntlet_compassFrame(u8 playerIndex)
{
    Actor *owner;
    Vector2Fp16 target;
    Vector2Int delta;
    if (!gauntlet_isRunning() || playerIndex != 0)
    {
        return 8;
    }
    owner = g_PlayerGlobals.players[0].ownerActor;
    if (owner == NULL)
    {
        return CompassFrame;
    }
    if (Gauntlet.phase == PHASE_TRAVEL)
    {
        CompassFoe = NULL;
        if (!gauntlet_travelTarget(&target))
        {
            return CompassFrame;
        }
    }
    else if (Gauntlet.phase == PHASE_COMBAT)
    {
        Actor *cursor;
        Actor *nearest = NULL;
        u32 distance = 0xffffffffu;
        for (cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
        {
            if (gauntlet_isLivingRunFoe(cursor))
            {
                u32 candidate = ABS((cursor->coords.x - owner->coords.x) >> 16) +
                                ABS((cursor->coords.y - owner->coords.y) >> 16);
                if (cursor == CompassFoe)
                {
                    nearest = cursor;
                    break;
                }
                if (candidate < distance)
                {
                    distance = candidate;
                    nearest = cursor;
                }
            }
        }
        CompassFoe = nearest;
        if (nearest == NULL)
        {
            return CompassFrame;
        }
        target = nearest->coords;
    }
    else
    {
        CompassFoe = NULL;
        return CompassFrame;
    }
    delta.x = (target.x - owner->coords.x) >> 16;
    delta.y = (target.y - owner->coords.y) >> 16;
    if (ABS(delta.x) > 8 || ABS(delta.y) > 8)
    {
        CompassFrame = math_vector2_direction8(delta);
    }
    return CompassFrame;
}

static bool32 floorLine(const Actor *footprint, const Actor *owner, const GauntletRoutePoint *point)
{
    s32 x = owner->coords.x >> 16;
    s32 y = owner->coords.y >> 16;
    s32 dx = point->x - x;
    s32 dy = point->y - y;
    u32 span = ABS(dx) > ABS(dy) ? ABS(dx) : ABS(dy);
    u32 steps = 1;
    u32 shift = 0;
    u32 i;
    if (span > 256)
    {
        return FALSE;
    }
    while (span > 8)
    {
        span = (span + 1) >> 1;
        steps <<= 1;
        shift++;
    }
    for (i = 1; i <= steps; i++)
    {
        Vector2Fp16 at = {(x + ((dx * (s32)i) >> shift)) * FP16_16_ONE,
                          (y + ((dy * (s32)i) >> shift)) * FP16_16_ONE};
        if (!hackLib_canLandAt(footprint, &at))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/** Keep cardinal and diagonal input safe while approaching a guide point. */
static bool32 guideReachable(const Actor *owner, const GauntletRoutePoint *point)
{
    Vector2Int size;
    s32 x = owner->coords.x >> 16;
    s32 y = owner->coords.y >> 16;
    s32 radius = ABS(owner->movementCollisionBox.xMin);
    s32 left;
    s32 right;
    s32 top;
    s32 bottom;
    s32 cx;
    s32 cy;
    if (ABS(point->x - x) > 128 || ABS(point->y - y) > 128)
    {
        return FALSE;
    }
    if (ABS(owner->movementCollisionBox.xMax) > radius)
    {
        radius = ABS(owner->movementCollisionBox.xMax);
    }
    left = (x < point->x ? x : point->x) - radius;
    right = (x > point->x ? x : point->x) + radius;
    top = (y < point->y ? y : point->y) + owner->movementCollisionBox.yMin;
    bottom = (y > point->y ? y : point->y) + owner->movementCollisionBox.yMax;
    terrain_getWorldSize(&size);
    if (left < 0 || top < 0 || right >= size.x || bottom >= size.y)
    {
        return FALSE;
    }
    for (cy = top / 8; cy <= bottom / 8; cy++)
    {
        for (cx = left / 8; cx <= right / 8; cx++)
        {
            if (terrain_getCollisionAt(cx * 8, cy * 8) & 7)
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

/** Rejoin the visible part of the path without requiring exact waypoint touches. */
static void refreshGuide(Actor *owner, const GauntletRoute *route)
{
    bool32 fallback = FALSE;
    u32 best = Gauntlet.waypoint;
    u32 distance = 0xffffffffu;
    u32 i;
    s32 x = owner->coords.x >> 16;
    s32 y = owner->coords.y >> 16;
retry:
    for (i = 0; i < route->count; i++)
    {
        s32 dx = route->points[i].x - x;
        s32 dy = route->points[i].y - y;
        u32 candidate = dx * dx + dy * dy;
        if (candidate <= distance && guideReachable(owner, &route->points[i]))
        {
            best = i;
            distance = candidate;
        }
    }
    // Broad activation can finish combat before the route's final corner.
    if (distance == 0xffffffffu && !fallback && Gauntlet.wave > 0)
    {
        route = &GauntletRoutes[Gauntlet.map][Gauntlet.wave - 1];
        fallback = TRUE;
        best = 0;
        goto retry;
    }
    while (best + 1 < route->count &&
           ABS(route->points[best + 1].x - x) <= GUIDE_LOOKAHEAD_PIXELS &&
           ABS(route->points[best + 1].y - y) <= GUIDE_LOOKAHEAD_PIXELS &&
           guideReachable(owner, &route->points[best + 1]))
    {
        best++;
    }
    GuideFallbackActive = fallback;
    if (fallback)
    {
        GuideFallback.x = route->points[best].x * FP16_16_ONE;
        GuideFallback.y = route->points[best].y * FP16_16_ONE;
    }
    else
    {
        Gauntlet.waypoint = best;
    }
    Gauntlet.guideTimer = GUIDE_RECHECK_TICKS;
}

/** The clearing and its connected approach form one broad activation zone. */
static bool32 canApproachSector(Actor *owner, const GauntletRoute *route)
{
    const GauntletRoutePoint *anchor = &route->points[route->count - 1];
    s32 x = owner->coords.x >> 16;
    s32 y = owner->coords.y >> 16;
    u32 distance = 0;
    u32 i = route->count - 1;
    if (ABS(anchor->x - x) >= SECTOR_APPROACH_PIXELS ||
        ABS(anchor->y - y) >= SECTOR_APPROACH_PIXELS)
    {
        return FALSE;
    }
    if (floorLine(&RouteFootprint, owner, anchor))
    {
        return TRUE;
    }
    while (i != 0)
    {
        distance += ABS(route->points[i].x - route->points[i - 1].x) +
                    ABS(route->points[i].y - route->points[i - 1].y);
        if (distance > SECTOR_ENTRY_PATH_PIXELS)
        {
            break;
        }
        i--;
        if (ABS(route->points[i].x - x) <= SECTOR_ENTRY_RADIUS &&
            ABS(route->points[i].y - y) <= SECTOR_ENTRY_RADIUS &&
            floorLine(&RouteFootprint, owner, &route->points[i]))
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void tickTravel(Actor *owner)
{
    const GauntletRoute *route = &GauntletRoutes[Gauntlet.map][Gauntlet.wave];
    Vector2Fp16 target;

    sweepActors();
    if (Gauntlet.entering)
    {
        if (Gauntlet.timer == ROOM_START_DELAY)
        {
            const GauntletRoutePoint *entry = &route->points[0];
            Vector2Fp16 position = {entry->x * FP16_16_ONE, entry->y * FP16_16_ONE};
            // Align the map entrance to floor that also fits the larger custom heroes.
            if (ABS(entry->x - (owner->coords.x >> 16)) <= 48 &&
                ABS(entry->y - (owner->coords.y >> 16)) <= 48 &&
                hackLib_canLandAt(owner, &position))
            {
                owner->renderCoords.x += position.x - owner->coords.x;
                owner->renderCoords.y += position.y - owner->coords.y;
                owner->coords = position;
            }
            announceRoom();
        }
        if (--Gauntlet.timer == 0)
        {
            Gauntlet.entering = FALSE;
        }
        return;
    }
    gauntlet_travelTarget(&target);
    if (Gauntlet.guideTimer == 0 || (ABS((target.x - owner->coords.x) >> 16) <= 12 &&
                                     ABS((target.y - owner->coords.y) >> 16) <= 12))
    {
        refreshGuide(owner, route);
    }
    else
    {
        Gauntlet.guideTimer--;
    }
    if (canApproachSector(owner, route))
    {
        Gauntlet.phase = PHASE_COMBAT;
        Gauntlet.wave++;
        Gauntlet.toSpawn = waveSize();
        Gauntlet.sectorSpawned = 0;
        Gauntlet.lastSpawnId = 0xff;
        Gauntlet.timer = Gauntlet.roles[Gauntlet.wave - 1] == ENCOUNTER_AMBUSH
                             ? SECTOR_WARNING_DELAY * 2
                             : SECTOR_WARNING_DELAY;
        announceWave();
        return;
    }
}

static void tickCombat(Actor *owner)
{
    u32 alive = sweepActors();

    if (Gauntlet.timer != 0)
    {
        Gauntlet.timer--;
        return;
    }
    if (Gauntlet.toSpawn != 0)
    {
        if (alive < aliveCap())
        {
            spawnFoe(owner);
            Gauntlet.toSpawn--;
            Gauntlet.timer =
                Gauntlet.roles[Gauntlet.wave - 1] == ENCOUNTER_AMBUSH ? 6 : SPAWN_INTERVAL;
        }
        return;
    }
    if (alive != 0)
    {
        return;
    }
    if (Gauntlet.wave == WAVES_PER_ROOM)
    {
        Gauntlet.phase = PHASE_SETTLING;
        Gauntlet.timer = ROOM_SETTLE_DELAY;
        return;
    }
    Gauntlet.phase = PHASE_TRAVEL;
    Gauntlet.waypoint = 0;
    Gauntlet.guideTimer = 0;
    GuideFallbackActive = FALSE;
    Gauntlet.timer = 0;
    skillExpansion_feedback(0, "Group cleared! Move on to the next fight.");
}

void gauntlet_playerTick(u8 playerIndex)
{
    Actor *owner;

    if (Gauntlet.mode != GAUNTLET_MODE_RUNNING || playerIndex != 0 ||
        g_SceneCurrent.id != SCENE_ID_GAMEPLAY || Gauntlet.phase == PHASE_LEAVING)
    {
        return;
    }
    owner = PLAYER(0).ownerActor;
    if (owner == NULL)
    {
        return;
    }
    gauntletTest_tick(owner);
    if (PLAYER(0).currentHp <= 0)
    {
        fall();
        return;
    }
    if (PLAYER(0).level > Gauntlet.lastLevel)
    {
        Gauntlet.levelPicks += PLAYER(0).level - Gauntlet.lastLevel;
        Gauntlet.lastLevel = PLAYER(0).level;
        gauntletLevel_settle(0);
        skillExpansion_feedback(0, "Level up!");
    }
    gauntletBoons_tick(0, owner, Gauntlet.room);
    if (Gauntlet.phase == PHASE_TRAVEL)
    {
        tickTravel(owner);
        return;
    }
    if (Gauntlet.phase == PHASE_SETTLING)
    {
        if (sweepActors() != 0)
        {
            Gauntlet.phase = PHASE_COMBAT;
            Gauntlet.timer = 0;
        }
        else if (--Gauntlet.timer == 0)
        {
            Gauntlet.phase = PHASE_CLEARED;
            Gauntlet.timer = ROOM_CLEAR_DELAY;
            skillExpansion_feedback(0, "Room cleared! Rewards await.");
        }
        return;
    }
    if (Gauntlet.phase == PHASE_CLEARED)
    {
        sweepActors();
        if (--Gauntlet.timer == 0)
        {
            clearRoom();
        }
        return;
    }
    tickCombat(owner);
}
