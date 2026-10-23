#pragma once

#include "gfx.h"
#include "types.h"
#include "vector2.h"

/** Don't include actor.h to avoid cycles, we only need pointers anyway */
typedef struct Actor Actor;

/** Relative offsets from the beginning of a mission group or record table. */
typedef struct MissionOffsetTable
{
    u16 count;
    u16 offsets[0];
} MissionOffsetTable;

typedef struct MissionChunkHeader
{
    u16 waypointSectionOffset;
    u16 scriptSectionOffset;
    u16 flags;
} MissionChunkHeader;

typedef struct MissionStreamSection
{
    u8 count;
    u8 field_0x1;
    u16 offsets[0];
} MissionStreamSection;

typedef struct MissionGroupSection
{
    u8 count;
    u8 field_0x1;
    u16 offsets[0];
} MissionGroupSection;

enum MissionFlag
{
    MISSION_FLAG_SHARED_CHUNK = 1 << 0,
};

typedef struct MissionWaypointRecord
{
    s16 x;
    s16 y;
    u32 field_0x4;
} MissionWaypointRecord;

typedef struct MissionWaypointPath
{
    u32 count;
    MissionWaypointRecord waypoints[0];
} MissionWaypointPath;

typedef struct MissionSpawnRecord
{
    s16 x;
    s16 y;
    u8 spawnPointId;
    u8 playerIndex;
    u8 field_0x6[2];
} MissionSpawnRecord;

typedef struct MissionBlockHeader
{
    u16 headerSize;
    u16 spawnRecordCount;
    MissionSpawnRecord spawnRecords[0];
} MissionBlockHeader;

typedef struct MissionChunkReference
{
    u16 offset;
    u8 field_0x2[6];
} MissionChunkReference;

typedef struct MissionVariantTable
{
    u8 chunkCount;
    u8 variantChunks[11];
    MissionChunkReference chunks[0];
} MissionVariantTable;

typedef enum MissionId
{
    MISSION_ANDUIN_BANKS = 0x00,
    MISSION_ANDUIN_RIVER_CAMPSITE = 0x01,
    MISSION_ANDUIN_RIVER_CORSAIR_CAMP = 0x02,
    MISSION_ANDUIN_RIVER_CORSAIR_CHASE = 0x03,
    MISSION_ORC_TOWER_TOP_ROOM = 0x04,
    MISSION_ORC_TOWER = 0x05,
    MISSION_CIRITH_UNGOL_AMBUSH = 0x06,
    MISSION_CIRITH_UNGOL_CAMPSITE = 0x07,
    MISSION_CIRITH_UNGOL_LEDGE = 0x08,
    MISSION_CIRITH_UNGOL_PARAPET = 0x09,
    MISSION_CIRITH_UNGOL_PASS = 0x0a,
    MISSION_CIRITH_UNGOL_LAIR_CAMPSITE = 0x0b,
    MISSION_CIRITH_UNGOL_STAIRS = 0x0c,
    MISSION_CIRITH_UNGOL_SUMMIT = 0x0d,
    MISSION_CORSAIR_FLAGSHIP_ABOVE = 0x0e,
    MISSION_CORSAIR_FLAGSHIP_BELOW = 0x0f,
    MISSION_CRACK_OF_DOOM = 0x10,
    MISSION_CRACK_OF_DOOM_EDGE_OF_VOLCANO = 0x11,
    MISSION_CRACK_OF_DOOM_TUNNEL = 0x12,
    MISSION_DIMHOLT_CAMPSITE = 0x13,
    MISSION_DIMHOLT_DWIMERBERG_BASE = 0x14,
    MISSION_DIMHOLT_DWIMERBERG_MT = 0x15,
    MISSION_DIMHOLT_GLEN = 0x16,
    MISSION_DIMHOLT_ROAD = 0x17,
    MISSION_ANDUIN_DOCKS = 0x18,
    MISSION_MINAS_TIRITH_DOCKS = 0x19,
    MISSION_MT_DOOM_CAMPSITE = 0x1a,
    MISSION_FOOT_OF_MT_DOOM = 0x1b,
    MISSION_MT_DOOM_SAMMATH_NAUR = 0x1c,
    MISSION_SLOPES_OF_MT_DOOM = 0x1d,
    MISSION_DUNHARROW_CAMPS = 0x1e,
    MISSION_DUNHARROW_CAMPSITE = 0x1f,
    MISSION_DUNHARROW_HILLS = 0x20,
    MISSION_DUNHARROW_HOLLOW = 0x21,
    MISSION_DUNHARROW_PLATEAU = 0x22,
    MISSION_DUNHARROW_RIVER_CROSSING = 0x23,
    MISSION_FANGORN_CAMPSITE = 0x24,
    MISSION_FANGORN_CLEARING = 0x25,
    MISSION_FANGORN_COPSE = 0x26,
    MISSION_FANGORN_ENTWASH = 0x27,
    MISSION_GORGOROTH_CAMPSITE = 0x28,
    MISSION_GORGOROTH_HILLS = 0x29,
    MISSION_GORGOROTH_ORC_ROAD = 0x2a,
    MISSION_GORGOROTH_PLAINS = 0x2b,
    MISSION_HELMS_DEEP_CAMPSITE = 0x2c,
    MISSION_HELMS_DEEP_CAVERNS = 0x2d,
    MISSION_HELMS_DEEP_FORTRESS = 0x2e,
    MISSION_HELMS_DEEP_VALLEY = 0x2f,
    MISSION_WHITE_MOUNTAINS_ICE_CAVES = 0x30,
    MISSION_MORIA_BALINS_TOMB = 0x31,
    MISSION_ISENGARD_CAMPSITE = 0x32,
    MISSION_ISENGARD_FLOODED_FIELD = 0x33,
    MISSION_ISENGARD_URUK_HATCHERY = 0x34,
    MISSION_ISENGARD_ORTHANC = 0x35,
    MISSION_ISENGARD_ORTHANC_TUNNEL = 0x36,
    MISSION_ISENGARD_TOWER_BASE = 0x37,
    MISSION_ISENGARD_TOWER_DOOR = 0x38,
    MISSION_ISENGARD_TOWER_LEVELS = 0x39,
    MISSION_ITHILIEN_BLACK_GATE = 0x3a,
    MISSION_ITHILIEN_BLACK_ROAD = 0x3b,
    MISSION_ITHILIEN_NORTH = 0x3c,
    MISSION_ITHILIEN_WATERFALL = 0x3d,
    MISSION_MINAS_TIRITH_BATTLEMENTS = 0x3e,
    MISSION_MINAS_TIRITH_BATTLEMENTS_COURT_OF_KINGS = 0x3f,
    MISSION_MINAS_TIRITH_BATTLEMENTS_GATES = 0x40,
    MISSION_MINAS_TIRITH_BATTLEMENTS_CITY_CENTER = 0x41,
    MISSION_MINAS_TIRITH_BATTLEMENTS_STREETS = 0x42,
    MISSION_MINAS_TIRITH_BATTLEMENTS_STEWARDS_TOMB = 0x43,
    MISSION_DENETHOR_FALL_BALCONY = 0x44,
    MISSION_MINAS_TIRITH_TIERS_GATES = 0x45,
    MISSION_MINAS_TIRITH_TIERS_CITY_CENTER = 0x46,
    MISSION_MINAS_TIRITH_TIERS_STREETS = 0x47,
    MISSION_MINAS_TIRITH_TIERS_TIER_FOUR = 0x48,
    MISSION_MINAS_TIRITH_TIERS_TIER_THREE = 0x49,
    MISSION_MORGUL_BRIDGE = 0x4a,
    MISSION_MORGUL_CAMPSITE = 0x4b,
    MISSION_MORGUL_CROSSROADS = 0x4c,
    MISSION_MORGUL_GATE = 0x4d,
    MISSION_MINAS_MORGUL = 0x4e,
    MISSION_VALE_OF_MORGUL = 0x4f,
    MISSION_MORGUL_VALLEY = 0x50,
    MISSION_MORIA_HALL = 0x51,
    MISSION_MORIA_HALL_TOP = 0x52,
    MISSION_MORIA_LONG_STAIRS = 0x53,
    MISSION_MORIA_TREASURE_HOARDS = 0x54,
    MISSION_PATHS_OF_THE_DEAD_BLACK_CHASM = 0x55,
    MISSION_PATHS_OF_THE_DEAD_CAMPSITE = 0x56,
    MISSION_PATHS_OF_THE_DEAD_DARK_PATHS = 0x57,
    MISSION_PATHS_OF_THE_DEAD_GREY_VAPORS = 0x58,
    MISSION_MORGUL_SECRET_CAVE = 0x59,
    MISSION_PELENNOR_BEACON_WEST_BEACON = 0x5a,
    MISSION_PELENNOR_BEACON_EAST_BEACON = 0x5b,
    MISSION_PELENNOR_BEACON_MINAS_ENTRANCE = 0x5c,
    MISSION_PELENNOR_BATTLE_EDGE = 0x5d,
    MISSION_PELENNOR_BATTLE_FALL_OF_ROHIRRIM = 0x5e,
    MISSION_PELENNOR_BATTLE_NORTH_FIELDS = 0x5f,
    MISSION_PELENNOR_BATTLE_HIDDEN_PLAINS = 0x60,
    MISSION_PELENNOR_BATTLE_WEST_FIELDS = 0x61,
    MISSION_PELENNOR_BATTLE_WITCH_KING = 0x62,
    MISSION_PELENNOR_FIELDS_MUMAKIL_ATTACK = 0x63,
    MISSION_PELENNOR_FIELDS_CAMPSITE = 0x64,
    MISSION_PELENNOR_FIELDS_THEODENS_CHARGE = 0x65,
    MISSION_PELENNOR_FIELDS_FOOTPATH = 0x66,
    MISSION_PELENNOR_FIELDS_FORK = 0x67,
    MISSION_PELENNOR_FIELDS_HARADRIM_CAMP = 0x68,
    MISSION_PELENNOR_FIELDS_MT_MINDOLLUIN = 0x69,
    MISSION_PELENNOR_FIELDS_MUMAKIL_CAMP = 0x6a,
    MISSION_PELENNOR_FIELDS_OUTPOST = 0x6b,
    MISSION_ROHAN_CAMPSITE = 0x6c,
    MISSION_ROHAN_EDORAS_FIELDS = 0x6d,
    MISSION_ROHAN_GAP = 0x6e,
    MISSION_ROHAN_DEEPING_OUTSKIRTS = 0x6f,
    MISSION_ROHAN_HILLS = 0x70,
    MISSION_ROHAN_PLAINS = 0x71,
    MISSION_ROHAN_SNOWBOURNE_TRAIL = 0x72,
    MISSION_SHELOBS_LAIR_CAVES = 0x73,
    MISSION_SHELOBS_LAIR = 0x74,
    MISSION_SHELOBS_LAIR_TUNNEL_MOUTH = 0x75,
    MISSION_WEATHERTOP_CAMPSITE = 0x76,
    MISSION_WEATHERTOP_HILLS = 0x77,
    MISSION_WEATHERTOP_MARSH = 0x78,
    MISSION_WEATHERTOP_TOWER = 0x79,
    MISSION_WHITE_MOUNTAINS_APPROACH = 0x7a,
    MISSION_WHITE_MOUNTAINS_CAMPSITE = 0x7b,
    MISSION_WHITE_MOUNTAINS_EDGE = 0x7c,
    MISSION_WHITE_MOUNTAINS_FOOTHILLS = 0x7d,
    MISSION_WHITE_MOUNTAINS_NARROW_PASS = 0x7e,
    MISSION_WHITE_MOUNTAINS_SE_MTS_EDGE = 0x7f,
    MISSION_WHITE_MOUNTAINS_HIDDEN_TRAIL = 0x80,
    MISSION_WHITE_MOUNTAINS_STORM = 0x81,
} MissionId;
/**
 * Index into RegionMissionRanges[28]: the region a mission plays in (also indexes
 * ItemTypeInfos[].prefixRegionRanges, picking the prefix band a drop rolls from).
 * Names follow the region-name text block (TEXT_ID_REGION_NAME_BASE + index).
 */
typedef enum RegionId
{
    REGION_ANDUIN_RIVER = 0,
    REGION_ORC_TOWER = 1,
    REGION_CIRITH_UNGOL = 2,
    REGION_CORSAIR_FLAGSHIP = 3,
    REGION_CRACK_OF_DOOM = 4,
    REGION_DIMHOLT = 5,
    REGION_DOCKS = 6,
    REGION_MT_DOOM = 7,
    REGION_DUNHARROW = 8,
    REGION_FANGORN = 9,
    REGION_GORGOROTH = 10,
    REGION_HELMS_DEEP = 11,
    REGION_ICE_CAVE = 12,
    REGION_ISENGARD = 13,
    REGION_ITHILIEN = 14,
    REGION_MINAS_TIRITH_BATTLEMENTS = 15,
    REGION_DENETHOR_FALL_BALCONY = 16,
    REGION_MINAS_TIRITH_TIERS = 17,
    REGION_MORGUL = 18,
    REGION_MORIA = 19,
    REGION_PATHS_OF_THE_DEAD = 20,
    REGION_PELENNOR_BEACON = 21,
    REGION_PELENNOR_BATTLE = 22,
    REGION_PELENNOR_FIELDS = 23,
    REGION_ROHAN = 24,
    REGION_SHELOBS_LAIR = 25,
    REGION_WEATHERTOP = 26,
    REGION_WHITE_MOUNTAINS = 27,
} __attribute__((packed)) RegionId;
/**
 * MissionTable @0x0806c0f0 row.
 * 12-byte stride: the u8 tail carries 3 padding bytes.
 */
typedef struct MissionTableEntry
{
    const PaletteCycleList *paletteCycles; // shared between missions, often NULL
    const u8 *block; // mission block (header, spawn records, variant table, chunks)
    u8 field_0x8;
} MissionTableEntry;
/**
 * Mission-id range selecting one region; RegionMissionRanges @0x0806d20c, scanned by
 * mission_getRegion.
 */
typedef struct RegionMissionRange
{
    u16 loMission;
    u16 hiMission;
} RegionMissionRange;
u32 mission_getRegion(u32 missionId);

/** Actor type identifiers. */
typedef enum ActorType
{
    ACTOR_TYPE_UNKNOWN_00 = 0x00,
    ACTOR_TYPE_GATE_PANEL = 2,        // animated tile gate visual; actor_gatePanel_*
    ACTOR_TYPE_MISSION_WARP = 3,      // walk-into-rect mission warp; actor_missionWarp_*
    ACTOR_TYPE_SWITCH = 4,            // two-position animated switch (lever / cycling door)
    ACTOR_TYPE_ENEMY_SPAWNER = 8,     // enemy spawner: 4 child slots, respawn-on-death, budget
    ACTOR_TYPE_ENEMY_SPAWNER_RNG = 9, // variant picking among 3 enemy ids by
                                      // probability; shares the spawner constructor, unused
                                      // in mission data
    ACTOR_TYPE_LOOT_PILE = 10,        // scripted ground pickup / generated loot pile;
                                      // actor_lootPile_*, a kind-dependent trigger callback
                                      // (actor_lootPile_trigger from kind 8 up)
    ACTOR_TYPE_CHEST = 12,            // R-press opens it, scatters drops; actor_chest_*
                                      // (unitType LOOT_UNIT_TYPE_CONTAINER)
    ACTOR_TYPE_GEM_RAIN = 16,         // gem-rain event object (as.gemRain): rains gem drops
                                      // over its x window, live-gem credits in currentHp
    ACTOR_TYPE_NPC = 20,              // non-enemy character/prop family (actor_npc_*): allies,
                                      // civilians, siege objects; Actor.variant picks the kind
                                      // (enum NpcKind)
    ACTOR_TYPE_ENEMY = 25,            // one enemy; Actor.variant is the kind: 0..2 roll one of
                                      // three units, 3.. map to a unitType via a table
                                      // @0x08283070 (kind 0xd = the flag bearer)
    ACTOR_TYPE_UNKNOWN_3E = 0x3e,
    ACTOR_TYPE_UNKNOWN_4F = 0x4f,
    ACTOR_TYPE_UNKNOWN_50 = 0x50,

    // Preserve the full unsigned 16-bit range.
    ACTOR_TYPE_STORAGE_MAX = 0xffff,
} __attribute__((packed)) ActorType;

/**
 * One spawn-group slot's actor params (mission-block data): the 8-byte head is
 * shared by all kinds; the payload past it is per spawn type, decoded by that kind's constructor
 * (Actor<Kind>MissionParams in include/actor/<Kind>.h).
 */
typedef struct ActorMissionParams
{
    // ActorType identifier stored as a word.
    u32 type;
    s16 x;
    s16 y;
} ActorMissionParams;

typedef struct MissionActorRecordHeader
{
    Actor *actor;
    u32 field_0x4;
} MissionActorRecordHeader;

typedef struct MissionActorGroupSource
{
    u8 count;
    u8 field_0x1;
    u8 recordSizes[0];
} MissionActorGroupSource;

/**
 * ActorMissionConstructors entry: build the engine object for one spawn-group slot.
 * NULL entries are unused spawn types. Each kind's params, its
 * union ActorKindState view and its prototypes live in include/actor/<Kind>.h.
 */
typedef struct Actor *(*ActorMissionConstructor)(u8 groupIndex, u8 actorIndex);

// Mission-script VM: each variant's streams run record sequences (opcode table
// MissionScriptOpcodeLengths, handlers MissionScriptOpcodeHandlers); actors spawn in
// per-group slots (params via mission_script_getActorParams, live actors via
// mission_script_getGroupActor). Streams and spawn groups share one slot index space per
// variant; slot 0 (and slot 1 when the mission shares chunk 0) is always loaded.
//
// Its globals sit at 0x03003228-0x0300324b: g_MissionCurrentStream, g_MissionFlags (the
// loaded chunk's u16 @+4; bit 0 = the mission shares chunk 0), g_MissionCheckpointVars
// (op 62 reads them, ops 63-66 write them), g_MissionScriptVars (scratch the
// read-modify-write ops 58-61 address as base + index), g_MissionVariant, and the
// g_MissionGroups / g_MissionWaypointPaths / g_MissionStreams areas inside the VM's
// 0x2c00-byte state arena addressed by g_MissionScriptState.

bool mission_script_slotNeedsLoad(u8 slotIndex);

/** Spawn a spawn group's actors, skipping slots that already have a live actor. */
void mission_script_spawnGroup(u8 groupIndex);
void mission_script_setRecordActor(Actor *actor, u8 groupIndex, u8 recordIndex);

void mission_script_runStream(u8 streamIndex);

void mission_script_spawnGroupAndStartStream(u8 groupIndex, u8 streamIndex);

/** Compare value with operand and start the pass or fail spawn group and stream pair. */
void mission_script_compareAndSpawn(u32 value, u32 operand, u32 op, u32 passGroup, u32 passStream, u32 failGroup,
                                    u32 failStream);

u16 mission_script_getGroupActorCount(u8 groupIndex);

ActorMissionParams *mission_script_getActorParams(u8 groupIndex, u8 actorIndex);

Actor *mission_script_getGroupActor(u8 groupIndex, u8 actorIndex);

/** Script opcode handlers (MissionScriptOpcodeHandlers[opcode - 1]); command points at the record. */
void mission_script_op01(const u8 *command);
void mission_script_op02(const u8 *command);
void mission_script_op03(const u8 *command);
void mission_script_op04(const u8 *command);
void mission_script_op14(const u8 *command);
void mission_script_op15(const u8 *command);
void mission_script_op46(const u8 *command);
void mission_script_op68(const u8 *command);
void mission_script_op05(const u8 *command);
void mission_script_op08(const u8 *command);
void mission_script_op19(const u8 *command);
void mission_script_op37(const u8 *command);
void mission_script_op67(const u8 *command);
void mission_script_op23(const u8 *command);
void mission_script_op44(const u8 *command);
void mission_script_op53(const u8 *command);
void mission_script_op56(const u8 *command);
void mission_script_op24(const u8 *command);
void mission_script_op48(const u8 *command);
void mission_script_op49(const u8 *command);
void mission_script_op51(const u8 *command);
void mission_script_op52(const u8 *command);
void mission_script_op38(const u8 *command);
void mission_script_op39(const u8 *command);
void mission_script_op40(const u8 *command);
void mission_script_op41(const u8 *command);
void mission_script_op17(const u8 *command);
void mission_script_op28(const u8 *command);
void mission_script_op50(const u8 *command);
void mission_script_op32(const u8 *command);
void mission_script_op34(const u8 *command);
void mission_script_op55(const u8 *command);
void mission_script_op27(const u8 *command);
void mission_script_op62(const u8 *command);
void mission_script_op42(const u8 *command);
void mission_script_op36(const u8 *command);
void mission_script_op21(const u8 *command);
void mission_script_op22(const u8 *command);
void mission_script_op45(const u8 *command);
void mission_script_op33(const u8 *command);
void mission_script_op57(const u8 *command);
void mission_script_op43(const u8 *command);
void mission_script_op06(const u8 *command);
void mission_script_op07(const u8 *command);
void mission_script_op09(const u8 *command);
void mission_script_op10(const u8 *command);
void mission_script_op11(const u8 *command);
void mission_script_op20(const u8 *command);
void mission_script_op25(const u8 *command);
void mission_script_op29(const u8 *command);
void mission_script_op47(const u8 *command);
void mission_script_op54(const u8 *command);
void mission_script_op58(const u8 *command);
void mission_script_op59(const u8 *command);
void mission_script_op60(const u8 *command);
void mission_script_op61(const u8 *command);
void mission_script_op63(const u8 *command);
void mission_script_op64(const u8 *command);
void mission_script_op65(const u8 *command);
void mission_script_op66(const u8 *command);
void mission_script_op12(const u8 *command);
void mission_script_op13(const u8 *command);
void mission_script_op16(const u8 *command);

/**
 * Allocate a countdown object (tick fn from MissionScriptTimerUpdates[kind]); script
 * op 12 arms it to fire mission_script_spawnGroupAndStartStream when the count expires.
 */
Actor *mission_script_allocTimer(u8 kind);

void mission_script_loadMissionBlock(const u8 *block);

u8 *mission_script_loadGroups(const u8 *chunk0, const u8 *chunk, u8 *dest);

u8 *mission_script_copyGroup(const u8 *groupSection, u8 *dest, u8 groupIndex);
void mission_script_copyActorRecord(const u8 *source, u8 *dest, u8 size);

u8 *mission_script_copySpawnRecords(const u8 *block, const u8 *chunk);

u8 *mission_script_loadWaypointPaths(const u8 *chunk0, const u8 *chunk, u8 *dest);

u8 mission_script_getWaypointCount(u8 pathIndex);

Vector2Int mission_script_getWaypoint(u8 pathIndex, u8 waypointIndex);

u8 *mission_script_loadStreams(const u8 *chunk0, const u8 *chunk, u8 *dest);

u8 *mission_script_copyStream(const u8 *scriptSection, u8 *dest, u8 streamIndex);

u16 mission_script_getAlignedHalfwordBytes(u16 count);
u16 mission_script_alignByteSize(u16 size);
u8 *mission_script_copyWaypointPath(const MissionOffsetTable *section, u8 *dest, u8 pathIndex);

/** Per-player scripted camera pan state. */
typedef struct MissionCameraPan
{
    // actor the pan follows, or the player's own actor
    struct Actor *target;

    // the target's velocity when the pan started
    Vector2Fp16 savedVelocity;

    u8 field_0xc[8];

    // pan speed in 16.16 fixed point
    s32 speed;

    // pan length in frames
    s32 frames;

    u16 duration;

    u16 field_0x1e;
    u8 field_0x20;
    u8 field_0x21;

    // pan destination, only used by the first player
    u8 destX;
    u8 destY;

    // nonzero while a pan is running
    u8 state;

    // the target's action state when the pan started
    u8 savedActionState;

    u8 field_0x26;
    u8 field_0x27;
    u8 field_0x28;
    u8 field_0x29;
    u8 field_0x2a[2];
} MissionCameraPan;

/**
 * Per-player camera pan state machine: drives the player's velocity and action state
 * through a pan, then starts the queued script stream when it completes.
 */
void mission_tickCameraPan(u8 playerIndex);

// Drain two pooled object lists back to the free pool (scene transition teardown).
void mission_unknown0803915c(void);
void fx_freeAllParticles(void);
