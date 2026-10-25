#pragma once

// Gauntlet: a roguelike mode started from the main menu (the GCN Link row). A run is a fresh hero
// fighting waves of foes room after room, each room on its own map, until it falls.

#include "actor.h"
#include "player.h"
#include "rng.h"
#include "types.h"

/** Gauntlet.mode values. */
enum GauntletMode
{
    GAUNTLET_MODE_OFF = 0,
    GAUNTLET_MODE_RUNNING = 1,
};

/** A gameplay-stream roll in [0, n). rng_mod's own range includes n. */
static inline u32 gauntlet_randBelow(u16 n)
{
    return rng_mod(n - 1);
}

/** Pick screen pages (the mod scene's arg 0). */
enum GauntletPage
{
    GAUNTLET_PAGE_BOON,
    GAUNTLET_PAGE_LEVEL,
    GAUNTLET_PAGE_RESULTS,
    GAUNTLET_PAGE_HUB,
    GAUNTLET_PAGE_UNLOCKS,
    GAUNTLET_PAGE_BRIEFING = 5,
    GAUNTLET_PAGE_DOORS = 6,
    GAUNTLET_PAGE_SHOP = 7,
    GAUNTLET_PAGE_SHOP_ITEM = 8,
    GAUNTLET_PAGE_SHOP_SELL = 9,
    GAUNTLET_PAGE_SHOP_SELL_ITEM = 10,
};

/** The kinds of room a door leads to. */
enum GauntletDoorType
{
    DOOR_BATTLE = 0,
    DOOR_ELITE = 1,
    DOOR_REST = 2,
    DOOR_TREASURE = 3,
    DOOR_SKIRMISH = 4,
    DOOR_ONSLAUGHT = 5,
    DOOR_SHRINE = 6,
    DOOR_TRIAL = 7,
    DOOR_TYPE_COUNT = 8,
};

enum GauntletRoomModifier
{
    ROOM_MODIFIER_NONE = 0,
    ROOM_MODIFIER_ARMORED = 1,
    ROOM_MODIFIER_RUSH = 2,
    ROOM_MODIFIER_SUPPLIES = 3,
};

/** Foe families a room can field. Any map can host any family. */
enum GauntletTheme
{
    THEME_GOBLINS,
    THEME_ORCS,
    THEME_URUKS,
    THEME_SPIDERS,
    THEME_MEN,
    THEME_SOUTH,
    THEME_WARGS,
    THEME_COUNT,
};

#define GAUNTLET_DOOR_COUNT 3

typedef struct GauntletDoor
{
    // enum GauntletDoorType
    u8 type;
    // RoomMaps index, unused for rooms without a fight
    u8 map;
    // enum GauntletTheme
    u8 theme;
    // enum GauntletRoomModifier
    u8 modifier;
} GauntletDoor;

static inline bool32 gauntlet_doorHasCombat(u32 type)
{
    return type == DOOR_BATTLE || type == DOOR_ELITE || type == DOOR_SKIRMISH ||
           type == DOOR_ONSLAUGHT || type == DOOR_TRIAL;
}

/** A door in 16 bits, for the scene's arguments. */
static inline u32 gauntlet_packDoor(const GauntletDoor *door)
{
    return door->type | door->theme << 3 | door->map << 6 | door->modifier << 11;
}

static inline GauntletDoor gauntlet_unpackDoor(u32 packed)
{
    return (GauntletDoor){.type = packed & 7,
                          .theme = (packed >> 3) & 7,
                          .map = (packed >> 6) & 31,
                          .modifier = (packed >> 11) & 3};
}

/** What taking a door sends the player to next. */
enum GauntletDoorResult
{
    // the briefing and then the room's map
    DOOR_RESULT_ROOM,
    // a free boon pick
    DOOR_RESULT_BOON,
    // another set of doors
    DOOR_RESULT_DOORS,
};

/** The doors on offer for the next room. The first room has none. */
void gauntlet_rollDoors(GauntletDoor doors[GAUNTLET_DOOR_COUNT]);

/** Go through a door. */
u32 gauntlet_takeDoor(const GauntletDoor *door);

const char *gauntlet_doorMapName(const GauntletDoor *door);
const char *gauntlet_doorThemeName(u32 theme);

/** enum GauntletDoorType of the room entered last. */
u32 gauntlet_roomType(void);
GauntletDoor gauntlet_roomDoor(void);
u32 gauntlet_rewardRarity(void);
u32 gauntlet_doorRewardRarity(const GauntletDoor *door);
const char *gauntlet_modifierName(u32 modifier);

/** Per-player tick hook (skillExpansion_playerTick) driving waves and rooms during a run. */
void gauntlet_playerTick(u8 playerIndex);
Actor *gauntlet_createRoomPlayer(u8 playerIndex);

/** TRUE while a run is in progress. */
bool32 gauntlet_isRunning(void);

/** XP award scale in percent: faster leveling during a run. */
s32 gauntlet_xpPercent(void);

/** Start a run with a fresh hero on the first room's map. */
void gauntlet_startRun(HeroId heroId);

/** TRUE for a living foe this run spawned. */
bool32 gauntlet_isLivingRunFoe(Actor *actor);

/** Count a foe slain by the player during a run. */
void gauntlet_noteKill(void);

// Run state for the pick screen (gauntlet_scene.c).

/** Rooms cleared so far. */
u32 gauntlet_roomsCleared(void);
u32 gauntlet_kills(void);

/** TRUE and one queued level-up taken when a level-up reward is still to be picked. */
bool32 gauntlet_takeLevelPick(void);

/** Leave the pick screen for the next room's map. */
void gauntlet_enterNextRoom(void);

/** Select the next map once so the briefing and loaded room agree. */
void gauntlet_prepareNextRoom(void);
const char *gauntlet_roomName(void);
const char *gauntlet_encounterName(void);
/** Next floor waypoint while exploring, also used by the automated traversal check. */
bool32 gauntlet_travelTarget(Vector2Fp16 *position);

/** Fresh herbs for a hero that has Herbal Healing, up to the pouch's limit. */
void gauntlet_addHerbs(u32 count);

/** TRUE when the run's hero has Herbal Healing, and so uses herbs. */
bool32 gauntlet_hasHerbs(void);

/** HeroId of the current or last run. */
u32 gauntlet_hero(void);

u32 gauntlet_rerollsLeft(void);

/** TRUE and one reroll spent when the run has one left. */
bool32 gauntlet_useReroll(void);

/** End the run state after the results screen. */
void gauntlet_finishRun(void);

/** Native compass frame, or 8 to preserve campaign tracking. */
u32 gauntlet_compassFrame(u8 playerIndex);
