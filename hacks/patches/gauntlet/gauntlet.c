#include "patches/gauntlet/gauntlet.h"

#include "actor/enemySpawner.h"
#include "actor/player.h"
#include "game.h"
#include "item.h"
#include "loot.h"
#include "lib/mission_scripts.h"
#include "lib/movement.h"
#include "math.h"
#include "memory.h"
#include "mission.h"
#include "mod_scenes.h"
#include "patches/gauntlet/gauntlet_boon_effects.h"
#include "patches/gauntlet/gauntlet_boons.h"
#include "patches/gauntlet/gauntlet_level.h"
#include "patches/gauntlet/gauntlet_meta.h"
#include "patches/gauntlet/gauntlet_routes.h"
#include "patches/gauntlet/gauntlet_shop.h"
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
#define ANIMATED_WATER_BACKDROP 4
#define ANIMATED_WATER_EFFECT 6
#define ROOM_START_DELAY 120
#define ROOM_SETTLE_DELAY 30
#define WAVE_WARNING_DELAY 30
// Ticks between one wave falling and the next arriving.
#define WAVE_BREATHER_DELAY 90
// Foes arrive just off screen, on a ring of this many pixels around the player.
#define ARRIVAL_RADIUS_X 140
#define ARRIVAL_RADIUS_Y 100
#define ARRIVAL_DIRECTIONS 16
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
    {MISSION_CORSAIR_FLAGSHIP_ABOVE, THEME_SOUTH, 4, "Corsair Deck"},
    {MISSION_CORSAIR_FLAGSHIP_BELOW, THEME_SOUTH, 4, "Corsair Hold"},
    {MISSION_DIMHOLT_GLEN, THEME_MEN, 2, "Dimholt Glen"},
    {MISSION_FOOT_OF_MT_DOOM, THEME_ORCS, 3, "Mount Doom Foot"},
    {MISSION_SLOPES_OF_MT_DOOM, THEME_ORCS, 3, "Mount Doom Slopes"},
    {MISSION_FANGORN_COPSE, THEME_URUKS, 1, "Fangorn Copse"},
    {MISSION_FANGORN_ENTWASH, THEME_URUKS, 1, "Entwash"},
    {MISSION_ISENGARD_URUK_HATCHERY, THEME_URUKS, 2, "Uruk Hatchery"},
    {MISSION_ISENGARD_TOWER_BASE, THEME_URUKS, 2, "Orthanc Base"},
    {MISSION_MINAS_TIRITH_BATTLEMENTS_STREETS, THEME_MEN, 4, "City Streets"},
    {MISSION_PATHS_OF_THE_DEAD_DARK_PATHS, THEME_MEN, 3, "Dark Paths"},
    {MISSION_PELENNOR_BATTLE_NORTH_FIELDS, THEME_SOUTH, 4, "North Fields"},
    {MISSION_PELENNOR_FIELDS_HARADRIM_CAMP, THEME_SOUTH, 4, "Haradrim Camp"},
    {MISSION_ROHAN_EDORAS_FIELDS, THEME_WARGS, 2, "Edoras Fields"},
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
    {11, THEME_MEN, 2},     {3, THEME_MEN, 3},      {2, THEME_MEN, 3},      {12, THEME_MEN, 4},
    {4, THEME_MEN, 4},      {0, THEME_SOUTH, 2},    {13, THEME_SOUTH, 3},   {1, THEME_SOUTH, 3},
    {14, THEME_SOUTH, 4},   {16, THEME_WARGS, 1},   {42, THEME_WARGS, 2},   {17, THEME_WARGS, 3},
    {43, THEME_WARGS, 3},   {18, THEME_WARGS, 4},
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
    u8 mapReady : 1;
    u8 entering : 1;
    // enum GauntletDoorType of the current room
    u8 roomType : 3;
    // enum GauntletTheme of the current room's foes
    u8 theme : 3;
    u8 roles[WAVES_PER_ROOM];
    u8 sectorSpawned : 5;
    u8 modifier : 2;
    u8 lastSpawnId;
};

static struct Gauntlet Gauntlet;
// Shared clearance keeps approach triggers consistent across hero animation bounds.
static const Actor RouteFootprint = {.movementCollisionBox = {-16, 16, -12, 9}};

_Static_assert(ARRAY_COUNT(RoomMaps) <= 32, "door map field holds five bits");
_Static_assert(sizeof(SceneInfo) == 0x4c, "native scene info stride");
_Static_assert(ARRAY_COUNT(RoomMaps) == GAUNTLET_ROUTE_MAP_COUNT, "each map needs floor routes");

bool32 gauntlet_isRunning(void)
{
    return Gauntlet.mode == GAUNTLET_MODE_RUNNING;
}

s32 gauntlet_xpPercent(void)
{
    return gauntlet_isRunning() ? RUN_XP_PERCENT * (100 + gauntletBoons_xpBonusPercent()) / 100
                                : 100;
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

#define STARTING_HERBS 3
// The stock pouch holds ten herbs of each kind.
#define MAX_HERBS 10

bool32 gauntlet_hasHerbs(void)
{
    // Heroes with their own skill kit, the Ringwraith and Saruman, spend that row on another skill.
    return skillExpansion_getHeroKit(PLAYER(0).heroId) == NULL;
}

/** Fresh herbs for a hero that has Herbal Healing. The pouch carries over between rooms. */
void gauntlet_addHerbs(u32 count)
{
    u32 fresh = PLAYER(0).kingsfoilHerbs.fresh + count;

    if (gauntlet_hasHerbs())
    {
        PLAYER(0).kingsfoilHerbs.fresh = fresh > MAX_HERBS ? MAX_HERBS : fresh;
    }
}

/** Heroes with Herbal Healing start with it, and with a few herbs. */
static void grantStartingHerbs(void)
{
    if (gauntlet_hasHerbs())
    {
        *skillExpansion_activePurchased(0, ACTIVE_SKILL_HERBAL_HEALING) = 1;
        *skillExpansion_activeLevel(0, ACTIVE_SKILL_HERBAL_HEALING) = 1;
        gauntlet_addHerbs(STARTING_HERBS);
    }
}

/** Restore cast cycling and keep starting quick bindings on learned skills. */
static void initializeStartingControls(void)
{
    u32 first = ACTIVE_SKILL_NONE;
    u32 slot;

    player_resetActiveSkillControls();
    for (u32 row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        if (row != ACTIVE_SKILL_HERBAL_HEALING && PLAYER_ACTIVE_SKILL(0, row).callback != NULL &&
            *skillExpansion_activeLevel(0, row) != 0)
        {
            first = row;
            break;
        }
    }
    if (first == ACTIVE_SKILL_NONE)
    {
        return;
    }
    for (slot = 0; slot < ARRAY_COUNT(PLAYER(0).quickSelectActiveSkills); slot++)
    {
        u32 row = PLAYER(0).quickSelectActiveSkills[slot];
        if (row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT ||
            PLAYER_ACTIVE_SKILL(0, row).callback == NULL ||
            *skillExpansion_activeLevel(0, row) == 0)
        {
            PLAYER(0).quickSelectActiveSkills[slot] = first;
        }
    }
    if (PLAYER(0).optionFlags.p & PLAYER_OPTION_FLAG_QUICK_SKILL_SELECT)
    {
        for (slot = 0; slot < ARRAY_COUNT(PLAYER(0).quickSelectActiveSkills); slot++)
        {
            if (PLAYER(0).quickSelectActiveSkills[slot] == first)
            {
                PLAYER(0).selectedActiveSkill.quickSlot = slot;
                break;
            }
        }
    }
    else
    {
        PLAYER(0).selectedActiveSkill.cycleSkillIndex = first;
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
    grantStartingHerbs();
    blessing = meta.unlocks[GAUNTLET_UNLOCK_BLESSING];
    if (blessing != 0)
    {
        gauntletBoons_grantRandom(blessing == 1 ? RARITY_COMMON : RARITY_RARE);
    }
    Gauntlet.rerolls = meta.unlocks[GAUNTLET_UNLOCK_FATE];
    Gauntlet.mending = meta.unlocks[GAUNTLET_UNLOCK_MENDING];
}

static bool32 isMapAvailable(u32 map)
{
    // City Streets has corrupt terrain in fresh Gauntlet loads.
    return GauntletRoutes[map][0].count != 0 &&
           RoomMaps[map].mission != MISSION_MINAS_TIRITH_BATTLEMENTS_STREETS;
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
            routed += isMapAvailable(map);
        }
        u32 pick = (Gauntlet.room - 1) % routed;
        for (map = 0; map < ARRAY_COUNT(RoomMaps); map++)
        {
            if (isMapAvailable(map) && pick-- == 0)
            {
                return map;
            }
        }
    }
    for (tries = 0; tries < 32; tries++)
    {
        map = gauntlet_randBelow(ARRAY_COUNT(RoomMaps));
        if (isMapAvailable(map) && RoomMaps[map].firstRoom <= Gauntlet.room &&
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
        Gauntlet.theme = RoomMaps[Gauntlet.map].theme;
        Gauntlet.roomType = DOOR_BATTLE;
        Gauntlet.mapReady = TRUE;
    }
}

const char *gauntlet_roomName(void)
{
    return RoomMaps[Gauntlet.map].name;
}

const char *gauntlet_doorThemeName(u32 theme)
{
    static const char *const Names[THEME_COUNT] = {
        "Goblins",          "Orcs",       "Uruk-hai", "Spiders", "Men of the East",
        "Men of the South", "Warg-riders"};
    return theme < THEME_COUNT ? Names[theme] : "";
}

const char *gauntlet_encounterName(void)
{
    return gauntlet_doorThemeName(Gauntlet.theme);
}

const char *gauntlet_doorMapName(const GauntletDoor *door)
{
    return RoomMaps[door->map].name;
}

u32 gauntlet_roomType(void)
{
    return Gauntlet.roomType;
}

GauntletDoor gauntlet_roomDoor(void)
{
    return (GauntletDoor){.type = Gauntlet.roomType,
                          .map = Gauntlet.map,
                          .theme = Gauntlet.theme,
                          .modifier = Gauntlet.modifier};
}

u32 gauntlet_doorRewardRarity(const GauntletDoor *door)
{
    if (door->type == DOOR_TRIAL)
    {
        return RARITY_EPIC;
    }
    if (door->type == DOOR_ELITE || door->type == DOOR_ONSLAUGHT || door->type == DOOR_TREASURE ||
        door->modifier == ROOM_MODIFIER_ARMORED || door->modifier == ROOM_MODIFIER_RUSH)
    {
        return RARITY_RARE;
    }
    return RARITY_COMMON;
}

u32 gauntlet_rewardRarity(void)
{
    GauntletDoor door = gauntlet_roomDoor();
    return gauntlet_doorRewardRarity(&door);
}

const char *gauntlet_modifierName(u32 modifier)
{
    static const char *const Names[] = {"", "Armored", "Rush", "Supplies"};
    return modifier < ARRAY_COUNT(Names) ? Names[modifier] : "";
}

/** Rooms from this one on can be an elite fight. */
#define ELITE_DOOR_ROOM 3
#define REST_HERBS 3

static void grantShrineItem(void)
{
    static const u8 Types[] = {ITEM_TYPE_ARMOR, ITEM_TYPE_CLOAK, ITEM_TYPE_GLOVES,
                               ITEM_TYPE_SHOES, ITEM_TYPE_ACCESSORY};
    Item item = EMPTY_ITEM;
    u32 oldTier = g_LootTier;
    u32 hero = PLAYER(0).heroId;

    // Shrine equipment follows run depth rather than the last campaign map's loot tier.
    g_LootTier = 1 + (Gauntlet.room - 1) / ROOMS_PER_TIER;
    if (g_LootTier > 10)
    {
        g_LootTier = 10;
    }
    item.d.itemType = Types[gauntlet_randBelow(ARRAY_COUNT(Types))];
    loot_rollBaseItem(&item, 0);
    // Native prefix progression tables cover only the eight campaign heroes.
    loot_rollPrefix(&item, 0xff, hero < 8 ? hero : HERO_ID_GANDALF,
                    LOOT_UNIT_TYPE_CONTAINER);
    loot_rollSuffix(&item, 0);
    g_LootTier = oldTier;
    if (!player_addItemToInventory(item, 0) && Gauntlet.rerolls < 255)
    {
        Gauntlet.rerolls++;
    }
}

static bool32 doorTakesMap(const GauntletDoor *doors, u32 filled, u32 map)
{
    u32 i;

    for (i = 0; i < filled; i++)
    {
        if (gauntlet_doorHasCombat(doors[i].type) && doors[i].map == map)
        {
            return TRUE;
        }
    }
    return map == Gauntlet.map;
}

static bool32 doorTakesTheme(const GauntletDoor *doors, u32 filled, u32 theme)
{
    u32 i;

    for (i = 0; i < filled; i++)
    {
        if (gauntlet_doorHasCombat(doors[i].type) && doors[i].theme == theme)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/** Recovery milestones and a weighted special choice between them. */
static u32 pickSpecialDoor(u32 room)
{
    static const u8 Types[] = {DOOR_TREASURE, DOOR_SHRINE, DOOR_ONSLAUGHT, DOOR_ONSLAUGHT};
    u32 type;
    if (room % 4 == 0)
    {
        return DOOR_REST;
    }
    if (PLAYER(0).currentHp * 100 <= PLAYER_STAT(0, STAT_MAX_HP) * 40 && gauntlet_randBelow(3) == 0)
    {
        return DOOR_REST;
    }
    type = Types[gauntlet_randBelow(ARRAY_COUNT(Types))];
    if ((room < 3 && type == DOOR_TREASURE) || type == Gauntlet.roomType)
    {
        type = DOOR_ONSLAUGHT;
    }
    return type;
}

void gauntlet_rollDoors(GauntletDoor doors[GAUNTLET_DOOR_COUNT])
{
    u32 room = Gauntlet.room;
    u32 slot;

    for (slot = 0; slot < GAUNTLET_DOOR_COUNT; slot++)
    {
        GauntletDoor *door = &doors[slot];
        u32 tries;

        *door = (GauntletDoor){.type = DOOR_BATTLE};
        if (slot == 1 && room >= ELITE_DOOR_ROOM)
        {
            door->type = DOOR_ELITE;
        }
        else if (slot == 1)
        {
            door->type = DOOR_SKIRMISH;
        }
        else if (slot == 2)
        {
            door->type = pickSpecialDoor(room);
        }
        if (slot == 1 && room >= ELITE_DOOR_ROOM && gauntlet_randBelow(2) == 0)
        {
            door->type = DOOR_SKIRMISH;
        }
        if (slot == 1 && room >= 6 && gauntlet_randBelow(3) == 0)
        {
            door->type = DOOR_TRIAL;
        }
        if (!gauntlet_doorHasCombat(door->type))
        {
            continue;
        }
        if (room >= 3 && door->type != DOOR_TRIAL && !gauntletTest_probesMaps())
        {
            door->modifier = gauntlet_randBelow(4);
        }
        door->map = Gauntlet.map;
        for (tries = 0; tries < 48; tries++)
        {
            u32 map = gauntlet_randBelow(ARRAY_COUNT(RoomMaps));

            if (isMapAvailable(map) && RoomMaps[map].firstRoom <= room &&
                !doorTakesMap(doors, slot, map))
            {
                door->map = map;
                break;
            }
        }
        door->theme = THEME_ORCS;
        for (tries = 0; tries < 48; tries++)
        {
            u32 theme = gauntlet_randBelow(THEME_COUNT);

            if (!((theme == THEME_MEN || theme == THEME_SOUTH) && room < 4) &&
                !(theme == THEME_WARGS && room < 2) && !doorTakesTheme(doors, slot, theme))
            {
                door->theme = theme;
                break;
            }
        }
        if (slot == 0 && gauntletTest_probesMaps())
        {
            door->map = pickMap();
            door->theme = RoomMaps[door->map].theme;
        }
    }
    // Map probes keep their forced map in the first slot.
    if (!gauntletTest_probesMaps())
    {
        for (slot = GAUNTLET_DOOR_COUNT - 1; slot > 0; slot--)
        {
            u32 pick = gauntlet_randBelow(slot + 1);
            GauntletDoor swap = doors[slot];
            doors[slot] = doors[pick];
            doors[pick] = swap;
        }
    }
}

static u32 roomWaveCount(void)
{
    return Gauntlet.roomType == DOOR_SKIRMISH || Gauntlet.roomType == DOOR_TRIAL ? 2
                                                                                 : WAVES_PER_ROOM;
}

u32 gauntlet_takeDoor(const GauntletDoor *door)
{
    Gauntlet.roomType = door->type;
    Gauntlet.modifier = door->modifier;
    if (door->type == DOOR_REST)
    {
        if (gauntlet_hasHerbs())
        {
            gauntlet_addHerbs(REST_HERBS);
        }
        else if (Gauntlet.rerolls < 255)
        {
            Gauntlet.rerolls++;
        }
        Gauntlet.room++;
        return DOOR_RESULT_DOORS;
    }
    if (door->type == DOOR_SHRINE)
    {
        grantShrineItem();
        Gauntlet.room++;
        return DOOR_RESULT_DOORS;
    }
    if (door->type == DOOR_TREASURE)
    {
        Gauntlet.room++;
        return DOOR_RESULT_BOON;
    }
    Gauntlet.map = door->map;
    Gauntlet.theme = door->theme;
    Gauntlet.mapReady = TRUE;
    return DOOR_RESULT_ROOM;
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
    for (u32 i = WAVES_PER_ROOM - 1; i > 0 && Gauntlet.roomType != DOOR_ELITE; i--)
    {
        u32 j = gauntlet_randBelow(i + 1);
        u8 role = Gauntlet.roles[i];
        Gauntlet.roles[i] = Gauntlet.roles[j];
        Gauntlet.roles[j] = role;
    }
    if (Gauntlet.roomType == DOOR_TRIAL)
    {
        for (u32 i = 0; i < WAVES_PER_ROOM; i++)
        {
            Gauntlet.roles[i] = ENCOUNTER_ELITE_GUARD;
        }
    }
    if (Gauntlet.roomType == DOOR_ELITE)
    {
        Gauntlet.roles[0] = Gauntlet.roles[2] = ENCOUNTER_ELITE_GUARD;
        Gauntlet.roles[1] = ENCOUNTER_AMBUSH;
    }
    if (Gauntlet.roomType == DOOR_ONSLAUGHT)
    {
        for (u32 i = 0; i < WAVES_PER_ROOM; i++)
        {
            Gauntlet.roles[i] = ENCOUNTER_AMBUSH;
        }
    }
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

    gauntletShop_reset();
    gauntletBoons_reset();
    memory_memset(skillExpansion_players, 0, sizeof(skillExpansion_players));
    g_GameFlags.p = GAME_FLAG_BONUS_MAP_ACTIVE;
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
    Gauntlet = (struct Gauntlet){.mode = GAUNTLET_MODE_RUNNING, .room = 1, .hero = heroId};
    Gauntlet.lastLevel = PLAYER(0).level;
    applyUnlocks();
    initializeStartingControls();
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
    gauntletShop_reset();
    loadRoom(Gauntlet.room == 1 ? GAMEPLAY_MODE_START_HEALED : GAMEPLAY_MODE_START);
}

void gauntlet_finishRun(void)
{
    gauntletShop_reset();
    gauntletBoonEffects_roomEnd();
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
    u32 tier = 1 + (Gauntlet.room - 1) / ROOMS_PER_TIER + (Gauntlet.roomType == DOOR_ELITE) +
               2 * (Gauntlet.roomType == DOOR_TRIAL);

    return tier > TIER_COUNT ? TIER_COUNT : tier;
}

/** A random foe of the room's theme, mostly from the highest tier the room allows. */
static u8 pickSpawnId(bool32 leader)
{
    u32 theme = Gauntlet.theme;
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

// Unit steps around the player in tenths of the arrival radii.
static const s8 ArrivalSteps[ARRIVAL_DIRECTIONS][2] = {
    {10, 0},  {9, 4},   {7, 7},   {4, 9},   {0, 10},  {-4, 9}, {-7, 7}, {-9, 4},
    {-10, 0}, {-9, -4}, {-7, -7}, {-4, -9}, {0, -10}, {4, -9}, {7, -7}, {9, -4},
};

static bool32 floorLine(const Actor *footprint, const Actor *owner,
                        const GauntletRoutePoint *point);

/**
 * A floor point just off screen from the player, starting at one direction and fanning around.
 * It prefers points the foe can walk straight to the player from.
 */
static bool32 findArrivalPoint(Actor *foe, Actor *owner, u32 direction, Vector2Fp16 *landing)
{
    static const u8 Scales[] = {100, 80, 120};
    u32 pass;

    for (pass = 0; pass < 2; pass++)
    {
        u32 scale;

        for (scale = 0; scale < ARRAY_COUNT(Scales); scale++)
        {
            u32 i;

            for (i = 0; i < ARRIVAL_DIRECTIONS; i++)
            {
                const s8 *step = ArrivalSteps[(direction + i) % ARRIVAL_DIRECTIONS];
                Vector2Fp16 preferred = {
                    owner->coords.x +
                        step[0] * ARRIVAL_RADIUS_X * Scales[scale] / 1000 * FP16_16_ONE,
                    owner->coords.y +
                        step[1] * ARRIVAL_RADIUS_Y * Scales[scale] / 1000 * FP16_16_ONE,
                };
                GauntletRoutePoint point;

                if (!hackLib_findLanding(foe, &preferred, landing))
                {
                    continue;
                }
                point.x = landing->x >> 16;
                point.y = landing->y >> 16;
                if (pass == 1 || floorLine(&RouteFootprint, owner, &point))
                {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

/** Spawn one defender of the wave just off screen, ready to come for the player. */
static void spawnFoe(Actor *owner)
{
    u32 role = Gauntlet.roles[Gauntlet.wave - 1];
    // A patrol or elite guard arrives from one side, an ambush from all around.
    u32 direction =
        (Gauntlet.room * 7 + Gauntlet.wave * 5) +
        (role == ENCOUNTER_AMBUSH ? Gauntlet.sectorSpawned * 3 : Gauntlet.sectorSpawned % 3);
    Actor spawner = {0};
    Vector2Fp16 landing;
    Actor *foe;
    bool32 leader = role == ENCOUNTER_ELITE_GUARD && Gauntlet.sectorSpawned == 0;
    u8 spawnId = pickSpawnId(leader);

    spawner.coords = owner->coords;
    Gauntlet.spawned++;
    foe = actor_enemySpawner_spawnChild(spawnId, &spawner);
    if (foe == NULL)
    {
        return;
    }
    foe->parentActor = NULL;
    foe->recordGroup = foe->recordIndex = RUN_FOE_RECORD;
    if (findArrivalPoint(foe, owner, direction % ARRIVAL_DIRECTIONS, &landing))
    {
        foe->renderCoords.x += landing.x - foe->coords.x;
        foe->renderCoords.y += landing.y - foe->coords.y;
        foe->coords = landing;
        if (leader)
        {
            s32 hp = foe->currentHp + foe->currentHp / 2;
            foe->currentHp = hp > 32767 ? 32767 : hp;
        }
        if (Gauntlet.modifier == ROOM_MODIFIER_ARMORED)
        {
            s32 hp = foe->currentHp + foe->currentHp / 4;
            foe->currentHp = hp > 32767 ? 32767 : hp;
        }
        Gauntlet.sectorSpawned++;
        Gauntlet.lastSpawnId = spawnId;
        return;
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
    size = 4 + Gauntlet.room / 2 + (Gauntlet.roomType == DOOR_ELITE);
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
    char *out = appendText(hudLine(), "Wave ");

    out = appendNumber(out, Gauntlet.wave);
    out = appendText(out, "/");
    out = appendNumber(out, roomWaveCount());
    out = appendText(out, ": ");
    appendText(out, EncounterNames[Gauntlet.roles[Gauntlet.wave - 1]]);
    skillExpansion_feedback(0, hudLine());
}

static void clearRoom(void)
{
    gauntletBoonEffects_roomEnd();
    u32 maxHp = PLAYER_STAT(0, STAT_MAX_HP);
    u32 heal = maxHp * (ROOM_CLEAR_HEAL_PERCENT + MENDING_HEAL_PERCENT * Gauntlet.mending) / 100;
    if (gauntletBoons_has(GAUNTLET_BOON_WRAITH_FORM))
    {
        heal /= 2;
    }
    if (Gauntlet.roomType == DOOR_TRIAL)
    {
        heal = 0;
    }
    u32 hp = PLAYER(0).currentHp + heal;

    PLAYER(0).currentHp = hp > maxHp ? maxHp : hp;
    if (Gauntlet.roomType == DOOR_BATTLE && Gauntlet.rerolls < 255)
    {
        Gauntlet.rerolls++;
    }
    // Elite rooms leave a herb behind.
    if (Gauntlet.roomType == DOOR_ELITE)
    {
        gauntlet_addHerbs(1);
    }
    if (Gauntlet.modifier == ROOM_MODIFIER_SUPPLIES)
    {
        if (gauntlet_hasHerbs())
        {
            gauntlet_addHerbs(2);
        }
        else
        {
            s32 cap = PLAYER_STAT(0, STAT_MAX_SPIRIT);
            if (PLAYER(0).currentSpirit < cap)
            {
                s32 spirit = PLAYER(0).currentSpirit + cap / 4;
                PLAYER(0).currentSpirit = spirit > cap ? cap : spirit;
            }
        }
    }
    Gauntlet.phase = PHASE_LEAVING;
    gauntletTest_roomCleared(Gauntlet.room, RoomMaps[Gauntlet.map].mission, Gauntlet.spawned,
                             Gauntlet.landFails, Gauntlet.purged);
    Gauntlet.room++;
    game_requestSceneChangeWithArgs(MOD_SCENE_ID(GAUNTLET), GAUNTLET_PAGE_BOON, 0);
}

static void fall(void)
{
    gauntletBoonEffects_roomEnd();
    Gauntlet.phase = PHASE_LEAVING;
    gauntletTest_runOver(Gauntlet.room, Gauntlet.kills);
    game_requestSceneChangeWithArgs(MOD_SCENE_ID(GAUNTLET), GAUNTLET_PAGE_RESULTS, 0);
}

bool32 gauntlet_travelTarget(Vector2Fp16 *position)
{
    (void)position;
    return FALSE;
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
    if (Gauntlet.phase == PHASE_COMBAT)
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

/** The next wave of the room arrives: a short warning, then its foes come for the player. */
static void beginWave(void)
{
    Gauntlet.phase = PHASE_COMBAT;
    Gauntlet.wave++;
    Gauntlet.toSpawn = waveSize();
    Gauntlet.sectorSpawned = 0;
    Gauntlet.lastSpawnId = 0xff;
    Gauntlet.timer = Gauntlet.roles[Gauntlet.wave - 1] == ENCOUNTER_AMBUSH ? WAVE_WARNING_DELAY * 2
                                                                           : WAVE_WARNING_DELAY;
    announceWave();
}

/** Pick a roomy entry from the map's connected floor routes. */
static void placePlayerEntry(Actor *owner)
{
    static const Actor Clearance = {.movementCollisionBox = {-24, 24, -20, 17}};
    const u32 candidatesPerRoute = 4;
    u32 count = GAUNTLET_SECTOR_COUNT * candidatesPerRoute;
    u32 start = gauntlet_randBelow(count);
    for (u32 attempt = 0; attempt < count; attempt++)
    {
        u32 candidate = (start + attempt) % count;
        const GauntletRoute *route = &GauntletRoutes[Gauntlet.map][candidate / candidatesPerRoute];
        if (route->count == 0)
        {
            continue;
        }
        u32 index = route->count * (candidate % candidatesPerRoute) / candidatesPerRoute;
        const GauntletRoutePoint *point = &route->points[index];
        Vector2Fp16 position = {point->x * FP16_16_ONE, point->y * FP16_16_ONE};
        Vector2Fp16 landing;
        if (!hackLib_findLanding(&Clearance, &position, &landing) ||
            !hackLib_canLandAt(owner, &landing))
        {
            continue;
        }
        owner->renderCoords.x += landing.x - owner->coords.x;
        owner->renderCoords.y += landing.y - owner->coords.y;
        owner->coords = landing;
        gauntletTest_roomEntered(Gauntlet.roomType, Gauntlet.map, candidate, owner);
        return;
    }
    gauntletTest_roomEntered(Gauntlet.roomType, Gauntlet.map, count, owner);
    // Keep the native mission entry if none of the wider candidates fits this hero.
}

/** Place the hero before native camera setup and the first terrain upload. */
Actor *gauntlet_createRoomPlayer(u8 playerIndex)
{
    Actor *owner = actor_player_create(playerIndex);
    if (owner != NULL && playerIndex == 0 && gauntlet_isRunning() && Gauntlet.entering)
    {
        placePlayerEntry(owner);
        // Campaign scripts initialize this visual backdrop even though it is not a story effect.
        if (g_CurrentMissionId < ARRAY_COUNT(SceneInfoTable) &&
            SceneInfoTable[g_CurrentMissionId].backdropKind == ANIMATED_WATER_BACKDROP)
        {
            scene_startTransitionEffect(ANIMATED_WATER_EFFECT);
        }
    }
    return owner;
}

/** Arrival at the room's entrance, then a breather before each wave. */
static void tickTravel(void)
{
    sweepActors();
    if (Gauntlet.entering)
    {
        if (Gauntlet.timer == ROOM_START_DELAY)
        {
            announceRoom();
        }
        if (--Gauntlet.timer == 0)
        {
            Gauntlet.entering = FALSE;
        }
        return;
    }
    if (Gauntlet.timer != 0)
    {
        Gauntlet.timer--;
        return;
    }
    beginWave();
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
    if (Gauntlet.wave == roomWaveCount())
    {
        Gauntlet.phase = PHASE_SETTLING;
        Gauntlet.timer = ROOM_SETTLE_DELAY;
        return;
    }
    Gauntlet.phase = PHASE_TRAVEL;
    Gauntlet.timer =
        Gauntlet.modifier == ROOM_MODIFIER_RUSH ? WAVE_BREATHER_DELAY / 3 : WAVE_BREATHER_DELAY;
    skillExpansion_feedback(0, "Wave down! The next one is coming.");
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
        tickTravel();
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
