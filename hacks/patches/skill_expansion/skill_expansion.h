#pragma once

// Skill Expansion: up to 11 active skills (plus Herbal Healing) and 18 passive skills per hero.
//
// Active rows 0..4 are stock, 5 is Herbal Healing, 6 stays ACTIVE_SKILL_NONE (a null-callback row)
// and 7..12 are new. Passive slots 0..8 are stock, 9..17 are new. Player.activeSkills,
// activeSkillIcons and passiveSkillIds point at per-player tables in mod RAM, so pointer-based game
// code sees the larger tables. Levels for new rows/slots live in shadow arrays here; use the
// accessors below instead of indexing Player arrays directly.

#include "hero_count.h"
#include "lib/cast_timeline.h"
#include "lib/combat_event.h"
#include "lib/combat_result.h"
#include "player.h"
#include "skill.h"
#include "stats.h"
#include "variables.h"

#define SKILL_EXPANSION_ACTIVE_ROW_COUNT 13
#define SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW 7
#define SKILL_EXPANSION_NEW_ACTIVE_COUNT 6

// Rows from ACTIVE_SKILL_NONE up keep their levels and cycle flags in the shadow arrays.
#define SKILL_EXPANSION_SHADOW_ACTIVE_COUNT (SKILL_EXPANSION_ACTIVE_ROW_COUNT - ACTIVE_SKILL_NONE)

#define SKILL_EXPANSION_PASSIVE_SLOT_COUNT 18
#define SKILL_EXPANSION_NEW_PASSIVE_COUNT                                                          \
    (SKILL_EXPANSION_PASSIVE_SLOT_COUNT - HERO_PASSIVE_SKILL_COUNT)

// Passive id of an empty tree slot, and findSlot's miss result.
#define SKILL_EXPANSION_PASSIVE_ID_NONE 0xff
#define SKILL_EXPANSION_PASSIVE_SLOT_NONE 0xff

#define SKILL_EXPANSION_HERO_COUNT HACK_HERO_COUNT

// 1: every defined new active row starts at level 1, for testing before the skill tree page exists.
#define SKILL_EXPANSION_TEST_GRANT_NEW_ACTIVES 0

// One new active row of a hero.
typedef enum SkillExpansionCastResult
{
    SKILL_CAST_SUCCESS = 0,
    SKILL_CAST_INVALID_TARGET = 1,
    SKILL_CAST_BLOCKED_LANDING = 2,
    SKILL_CAST_SUMMON_LIMIT = 3,
    SKILL_CAST_UNAVAILABLE = 4,
    SKILL_CAST_INTERRUPTED = 5,
} SkillExpansionCastResult;

typedef SkillExpansionCastResult (*SkillExpansionResolver)(Actor *actor);

#define SKILL_EXPANSION_RULE_NONE 0xff

typedef struct SkillExpansionCastPolicy
{
    // Animation-bank pose, independent of the callback's borrowed row.
    u8 pose;
    // Stock action-state bit, or SKILL_EXPANSION_RULE_NONE.
    u8 nativeCooldownRow;
    // Stock equipment/summon requirement row, or SKILL_EXPANSION_RULE_NONE.
    u8 requirementRow;
    // Per-skill cooldown begins on successful completion.
    u16 cooldownFrames;
} SkillExpansionCastPolicy;

typedef struct SkillExpansionActiveDef
{
    // Copied into the player's table (cost, curves, cast flags, trigger frame).
    const ActiveSkill *row;

    // Non-NULL: a stock callback that reads baseRow's level and table row. It runs with this row's
    // level and data swapped into baseRow. NULL: row->callback reads this row itself.
    ActiveSkillCallback stockCallback;

    // LZ77 icon asset for the HUD and skill tree.
    const void *icon;

    // Hero level required per purchased level, -1 once maxed. NULL: baseRow's requirements.
    const s8 *requiredLevels;

    // Skill tree description with @1 (required level) and @2 (spirit cost). NULL: baseRow's text.
    const char *description;

    // Extra cast condition, or NULL for none.
    bool32 (*canCast)(u8 playerIndex);

    // Optional result-returning callback instead of row->callback.
    SkillExpansionResolver resolve;

    // Stock row this skill borrows its cast pose, cooldown bit and cast requirements from.
    u8 baseRow;
    const SkillExpansionCastPolicy *policy;
    // Permanent save identity. Zero uses the registered legacy identity.
    u16 id;
    const HackLibCastTimeline *timeline;
    // Optional shared delivery recipe instead of a custom resolver or callback.
    const struct SkillDeliveryDef *delivery;
} SkillExpansionActiveDef;

// Passive skill IDs past the stock PassiveSkills table. They have no stat records, their effects
// read the skill level directly.
enum SkillExpansionPassiveId
{
    SKILL_EXPANSION_PASSIVE_STRIDERS_RIPOSTE = PASSIVE_SKILL_COUNT,
    SKILL_EXPANSION_PASSIVE_BLADE_REFORGED = PASSIVE_SKILL_COUNT + 1,
    SKILL_EXPANSION_PASSIVE_HUNTERS_PATIENCE = PASSIVE_SKILL_COUNT + 2,
    SKILL_EXPANSION_PASSIVE_KINGS_PRESENCE = PASSIVE_SKILL_COUNT + 3,
    SKILL_EXPANSION_PASSIVE_HANDS_OF_THE_KING = PASSIVE_SKILL_COUNT + 4,
    SKILL_EXPANSION_PASSIVE_RESTLESS_DEAD = PASSIVE_SKILL_COUNT + 5,
    SKILL_EXPANSION_PASSIVE_TRACK_THE_QUARRY = PASSIVE_SKILL_COUNT + 6,
    SKILL_EXPANSION_PASSIVE_THE_DEAD_RIDE = PASSIVE_SKILL_COUNT + 7,
    SKILL_EXPANSION_PASSIVE_LORD_OF_THE_DEAD = PASSIVE_SKILL_COUNT + 8,
    SKILL_EXPANSION_PASSIVE_SERVANT_OF_THE_RING = PASSIVE_SKILL_COUNT + 9,
    SKILL_EXPANSION_PASSIVE_WRAITH_FORM = PASSIVE_SKILL_COUNT + 10,
    SKILL_EXPANSION_PASSIVE_LORD_OF_TERROR = PASSIVE_SKILL_COUNT + 11,
    SKILL_EXPANSION_PASSIVE_SHADOW_HUNGER = PASSIVE_SKILL_COUNT + 12,
    SKILL_EXPANSION_PASSIVE_MORGUL_SORCERY = PASSIVE_SKILL_COUNT + 13,
    SKILL_EXPANSION_PASSIVE_FEARS_REACH = PASSIVE_SKILL_COUNT + 14,
    SKILL_EXPANSION_PASSIVE_WITCH_KINGS_MACE = PASSIVE_SKILL_COUNT + 15,
    SKILL_EXPANSION_PASSIVE_UNDYING_MALICE = PASSIVE_SKILL_COUNT + 16,
    SKILL_EXPANSION_PASSIVE_RINGBOUND = PASSIVE_SKILL_COUNT + 17,
    SKILL_EXPANSION_PASSIVE_HOST_OF_ANGMAR = PASSIVE_SKILL_COUNT + 18,
    SKILL_EXPANSION_PASSIVE_GRAVE_PACT = PASSIVE_SKILL_COUNT + 19,
    SKILL_EXPANSION_PASSIVE_MASTER_OF_THE_DEAD = PASSIVE_SKILL_COUNT + 20,
    SKILL_EXPANSION_PASSIVE_FADING = PASSIVE_SKILL_COUNT + 21,
    SKILL_EXPANSION_PASSIVE_KNIFE_IN_THE_DARK = PASSIVE_SKILL_COUNT + 22,
    SKILL_EXPANSION_PASSIVE_DEATH_FROM_SHADOW = PASSIVE_SKILL_COUNT + 23,
    SKILL_EXPANSION_PASSIVE_DREAD_LORD = PASSIVE_SKILL_COUNT + 24,
    SKILL_EXPANSION_PASSIVE_FEAST_OF_FEAR = PASSIVE_SKILL_COUNT + 25,
    SKILL_EXPANSION_PASSIVE_ENDLESS_NIGHT = PASSIVE_SKILL_COUNT + 26,
    SKILL_EXPANSION_PASSIVE_WEIGHT_OF_THE_RING = PASSIVE_SKILL_COUNT + 27,
    SKILL_EXPANSION_PASSIVE_STRENGTH_OF_WILL = PASSIVE_SKILL_COUNT + 28,
    SKILL_EXPANSION_PASSIVE_RIDDLES_IN_THE_DARK = PASSIVE_SKILL_COUNT + 29,
    SKILL_EXPANSION_PASSIVE_RINGS_HUNGER = PASSIVE_SKILL_COUNT + 30,
    SKILL_EXPANSION_PASSIVE_STING_GLOWS_BLUE = PASSIVE_SKILL_COUNT + 31,
    SKILL_EXPANSION_PASSIVE_TRICKSY_HOBBIT = PASSIVE_SKILL_COUNT + 32,
    SKILL_EXPANSION_PASSIVE_CLAIM_IT = PASSIVE_SKILL_COUNT + 33,
    SKILL_EXPANSION_PASSIVE_UNWAVERING = PASSIVE_SKILL_COUNT + 34,
    SKILL_EXPANSION_PASSIVE_PRECIOUS_LIES = PASSIVE_SKILL_COUNT + 35,
    SKILL_EXPANSION_PASSIVE_POOR_SMEAGOL = PASSIVE_SKILL_COUNT + 36,
    SKILL_EXPANSION_PASSIVE_HUNGRY = PASSIVE_SKILL_COUNT + 37,
    SKILL_EXPANSION_PASSIVE_SNIFF_IT_OUT = PASSIVE_SKILL_COUNT + 38,
    SKILL_EXPANSION_PASSIVE_NICE_MASTER = PASSIVE_SKILL_COUNT + 39,
    SKILL_EXPANSION_PASSIVE_RAT_CATCHER = PASSIVE_SKILL_COUNT + 40,
    SKILL_EXPANSION_PASSIVE_FINDERS_KEEPERS = PASSIVE_SKILL_COUNT + 41,
    SKILL_EXPANSION_PASSIVE_SMEAGOL_IS_FREE = PASSIVE_SKILL_COUNT + 42,
    SKILL_EXPANSION_PASSIVE_WE_WANTS_IT = PASSIVE_SKILL_COUNT + 43,
    SKILL_EXPANSION_PASSIVE_MY_PRECIOUS = PASSIVE_SKILL_COUNT + 44,
    SKILL_EXPANSION_PASSIVE_GREEN_THUMB = PASSIVE_SKILL_COUNT + 45,
    SKILL_EXPANSION_PASSIVE_MASTER_GARDENER = PASSIVE_SKILL_COUNT + 46,
    SKILL_EXPANSION_PASSIVE_GAFFERS_LORE = PASSIVE_SKILL_COUNT + 47,
    SKILL_EXPANSION_PASSIVE_LEMBAS_CRUMBS = PASSIVE_SKILL_COUNT + 48,
    SKILL_EXPANSION_PASSIVE_TATERS_FOR_ALL = PASSIVE_SKILL_COUNT + 49,
    SKILL_EXPANSION_PASSIVE_FINE_SEASONING = PASSIVE_SKILL_COUNT + 50,
    SKILL_EXPANSION_PASSIVE_STUBBORN = PASSIVE_SKILL_COUNT + 51,
    SKILL_EXPANSION_PASSIVE_I_CAN_CARRY_YOU = PASSIVE_SKILL_COUNT + 52,
    SKILL_EXPANSION_PASSIVE_SHIRE_GRIT = PASSIVE_SKILL_COUNT + 53,
    SKILL_EXPANSION_PASSIVE_LORE_OF_COLOURS = PASSIVE_SKILL_COUNT + 54,
    SKILL_EXPANSION_PASSIVE_FIRES_OF_INDUSTRY = PASSIVE_SKILL_COUNT + 55,
    SKILL_EXPANSION_PASSIVE_CUTPURSE = PASSIVE_SKILL_COUNT + 56,
    SKILL_EXPANSION_PASSIVE_RING_LORE = PASSIVE_SKILL_COUNT + 57,
    SKILL_EXPANSION_PASSIVE_MACHINE_OF_WAR = PASSIVE_SKILL_COUNT + 58,
    SKILL_EXPANSION_PASSIVE_SPITE = PASSIVE_SKILL_COUNT + 59,
    SKILL_EXPANSION_PASSIVE_HEAD_OF_THE_COUNCIL = PASSIVE_SKILL_COUNT + 60,
    SKILL_EXPANSION_PASSIVE_POWDER_OF_ORTHANC = PASSIVE_SKILL_COUNT + 61,
    SKILL_EXPANSION_PASSIVE_PARTING_SHOT = PASSIVE_SKILL_COUNT + 62,
    SKILL_EXPANSION_PASSIVE_DWARVEN_MAIL = PASSIVE_SKILL_COUNT + 63,
    SKILL_EXPANSION_PASSIVE_GRUDGEBEARER = PASSIVE_SKILL_COUNT + 64,
    SKILL_EXPANSION_PASSIVE_FRIENDLY_WAGER = PASSIVE_SKILL_COUNT + 65,
    SKILL_EXPANSION_PASSIVE_NOT_WITHOUT_ME = PASSIVE_SKILL_COUNT + 66,
    SKILL_EXPANSION_PASSIVE_BLOOD_ON_THE_AXE = PASSIVE_SKILL_COUNT + 67,
    SKILL_EXPANSION_PASSIVE_ORC_COUNT = PASSIVE_SKILL_COUNT + 68,
    SKILL_EXPANSION_PASSIVE_STUBBORN_AS_STONE = PASSIVE_SKILL_COUNT + 69,
    SKILL_EXPANSION_PASSIVE_DURINS_FOLK = PASSIVE_SKILL_COUNT + 70,
    SKILL_EXPANSION_PASSIVE_FORTY_TWO = PASSIVE_SKILL_COUNT + 71,
    SKILL_EXPANSION_PASSIVE_WANDERING_WIZARD = PASSIVE_SKILL_COUNT + 72,
    SKILL_EXPANSION_PASSIVE_ROBES_OF_WHITE = PASSIVE_SKILL_COUNT + 73,
    SKILL_EXPANSION_PASSIVE_KINDLER_OF_HEARTS = PASSIVE_SKILL_COUNT + 74,
    SKILL_EXPANSION_PASSIVE_OLD_TRICKS = PASSIVE_SKILL_COUNT + 75,
    SKILL_EXPANSION_PASSIVE_UNBENDING = PASSIVE_SKILL_COUNT + 76,
    SKILL_EXPANSION_PASSIVE_QUICKENED = PASSIVE_SKILL_COUNT + 77,
    SKILL_EXPANSION_PASSIVE_THE_FALL = PASSIVE_SKILL_COUNT + 78,
    SKILL_EXPANSION_PASSIVE_WHITE_RIDER = PASSIVE_SKILL_COUNT + 79,
    SKILL_EXPANSION_PASSIVE_SECRET_FIRE = PASSIVE_SKILL_COUNT + 80,
    SKILL_EXPANSION_PASSIVE_ROBES_OF_MANY_COLOURS = PASSIVE_SKILL_COUNT + 81,
    SKILL_EXPANSION_PASSIVE_KEEPER_OF_ORTHANC = PASSIVE_SKILL_COUNT + 82,
    SKILL_EXPANSION_PASSIVE_WISDOM_OF_THE_ISTARI = PASSIVE_SKILL_COUNT + 83,
    SKILL_EXPANSION_PASSIVE_KINDLING = PASSIVE_SKILL_COUNT + 84,
    SKILL_EXPANSION_PASSIVE_WILDFIRE = PASSIVE_SKILL_COUNT + 85,
    SKILL_EXPANSION_PASSIVE_STAFF_OF_ORTHANC = PASSIVE_SKILL_COUNT + 86,
    SKILL_EXPANSION_PASSIVE_HONEYED_WORDS = PASSIVE_SKILL_COUNT + 87,
    SKILL_EXPANSION_PASSIVE_VOICE_OF_COMMAND = PASSIVE_SKILL_COUNT + 88,
    SKILL_EXPANSION_PASSIVE_TREACHERY = PASSIVE_SKILL_COUNT + 89,
    SKILL_EXPANSION_PASSIVE_ELF_SIGHT = PASSIVE_SKILL_COUNT + 90,
    SKILL_EXPANSION_PASSIVE_SWIFT_NOCK = PASSIVE_SKILL_COUNT + 91,
    SKILL_EXPANSION_PASSIVE_ELVEN_CLOAK = PASSIVE_SKILL_COUNT + 92,
    SKILL_EXPANSION_PASSIVE_STEADY_HAND = PASSIVE_SKILL_COUNT + 93,
    SKILL_EXPANSION_PASSIVE_FLETCHER = PASSIVE_SKILL_COUNT + 94,
    SKILL_EXPANSION_PASSIVE_POINT_BLANK = PASSIVE_SKILL_COUNT + 95,
    SKILL_EXPANSION_PASSIVE_UNERRING = PASSIVE_SKILL_COUNT + 96,
    SKILL_EXPANSION_PASSIVE_TWIN_NOCK = PASSIVE_SKILL_COUNT + 97,
    SKILL_EXPANSION_PASSIVE_UNSHAKEN = PASSIVE_SKILL_COUNT + 98,
    SKILL_EXPANSION_PASSIVE_END = PASSIVE_SKILL_COUNT + 99,
};

#define SKILL_EXPANSION_ELITE_PASSIVE_COUNT (SKILL_EXPANSION_PASSIVE_END - PASSIVE_SKILL_COUNT)

// Hero level required per purchased level of a passive, -1 once maxed.
#define SKILL_EXPANSION_REQUIRED_LEVEL_COUNT (PASSIVE_SKILL_MAX_LEVEL + 1)

typedef struct SkillExpansionElitePassive
{
    // maxLevel, initialRequiredLevel and the stat records are used.
    PassiveSkill skill;

    // Hero level required per purchased level, -1 once maxed.
    s8 requiredLevels[SKILL_EXPANSION_REQUIRED_LEVEL_COUNT];

    // Stock passive icon index into PassiveSkillIconAssets, used when icon is NULL.
    u8 iconPassiveSkillId;

    // Custom raw tile icon (32x24 4bpp in the shared skill palette), or NULL.
    const void *icon;

    // Skill tree description with @1 (required level).
    const char *description;
} SkillExpansionElitePassive;

extern const SkillExpansionElitePassive
    SkillExpansionElitePassives[SKILL_EXPANSION_ELITE_PASSIVE_COUNT];

// A hero whose stock page rows and passive slots come from mod tables instead of the stock blocks.
typedef struct SkillExpansionHeroKit
{
    // HERO_ACTIVE_SKILL_COUNT rows. Row ACTIVE_SKILL_HERBAL_HEALING replaces Herbal Healing.
    const ActiveSkill *activeSkills;

    // Icon per row.
    const void *const *icons;

    // Hero level required per row and purchased level, -1 once maxed.
    const s8 (*requiredLevels)[SKILL_EXPANSION_REQUIRED_LEVEL_COUNT];

    // Skill tree description per row with @1 (required level) and @2 (spirit cost).
    const char *const *descriptions;

    // Extra cast condition of a row.
    bool32 (*canCast)(u8 playerIndex, u32 row);

    // HERO_PASSIVE_SKILL_COUNT passive skill IDs.
    const u8 *passiveSkillIds;

    const SkillExpansionActiveDef *newActives[SKILL_EXPANSION_NEW_ACTIVE_COUNT];
    u8 newPassiveSkillIds[SKILL_EXPANSION_NEW_PASSIVE_COUNT];

    // Stock hero whose skill tree tile art this hero uses.
    u8 artHeroId;

    // Transient state lifecycle. Purchased skills and equipment stats are separate.
    void (*reset)(u8 playerIndex);
    void (*cleanup)(u8 playerIndex, Actor *actor);
    void (*tick)(u8 playerIndex, Actor *actor);
    void (*refundSpirit)(u8 playerIndex, s32 amount);
    s32 (*meleeDamage)(u8 playerIndex, Actor *victim, s32 damage);
    s32 (*rangedDamage)(u8 playerIndex, Actor *victim, s32 damage);
    void (*onHit)(u8 playerIndex, Actor *attacker, Actor *victim, HackLibDamageResult result,
                  bool melee);
    void (*onKill)(u8 playerIndex, Actor *attacker, Actor *victim);
    s32 (*incomingDamage)(u8 playerIndex, Actor *attacker, s32 damage);
    void (*onHpChange)(u32 playerIndex, s32 amount, u32 flinchOnDamage);
    SkillExpansionResolver resolvers[HERO_ACTIVE_SKILL_COUNT];
    const SkillExpansionCastPolicy *policies[HERO_ACTIVE_SKILL_COUNT];
    // Optional permanent IDs for first-page skills, including Herbal Healing.
    const u16 *activeIds;
    // Optional timeline per expanded active row.
    const HackLibCastTimeline *const *timelines;
    // Optional gameplay HUD overlay, drawn after the stock HUD each tick.
    void (*drawHud)(u8 playerIndex);
    // Synchronous outgoing hit or miss, after the legacy kill and hit callbacks.
    void (*onCombatEvent)(u8 playerIndex, const HackLibCombatEvent *event);
    // Incoming hit or miss after native defenses and HP changes, before Strider's Riposte.
    void (*onIncomingCombatEvent)(u8 playerIndex, const HackLibCombatEvent *event);
} SkillExpansionHeroKit;

/** The hero's kit, or NULL for a hero built from the stock blocks. */
const SkillExpansionHeroKit *skillExpansion_getHeroKit(u32 heroId);
void skillExpansion_initHeroLoadout(u32 heroId, u32 playerIndex);
bool32 skillExpansion_activeCanCast(u8 playerIndex, u8 activeSkillIndex);

/** Skill tree tile background of the player's hero. */
const void *skillExpansion_getHeroTileBgAsset(u8 playerIndex);

typedef struct SkillExpansionPlayer
{
    ActiveSkill activeSkills[SKILL_EXPANSION_ACTIVE_ROW_COUNT];
    const void *activeSkillIcons[SKILL_EXPANSION_ACTIVE_ROW_COUNT];
    const SkillExpansionActiveDef *activeDefs[SKILL_EXPANSION_NEW_ACTIVE_COUNT];
    u8 passiveSkillIds[SKILL_EXPANSION_PASSIVE_SLOT_COUNT];

    s8 passiveSkillLevelsPurchased[SKILL_EXPANSION_NEW_PASSIVE_COUNT];  // passive slots 9..17
    s8 passiveSkillLevelsApplied[SKILL_EXPANSION_NEW_PASSIVE_COUNT];    // passive slots 9..17
    u8 activeSkillLevelsPurchased[SKILL_EXPANSION_SHADOW_ACTIVE_COUNT]; // active rows 6..12
    u8 activeSkillLevels[SKILL_EXPANSION_SHADOW_ACTIVE_COUNT];          // active rows 6..12
    u8 activeSkillCycleEnabled[SKILL_EXPANSION_SHADOW_ACTIVE_COUNT];    // active rows 6..12
} SkillExpansionPlayer;

extern SkillExpansionPlayer skillExpansion_players[2];

// Content tables (skill_expansion_content.c). NULL / SKILL_EXPANSION_PASSIVE_ID_NONE = empty.
extern const SkillExpansionActiveDef *const
    SkillExpansionActiveDefs[SKILL_EXPANSION_HERO_COUNT][SKILL_EXPANSION_NEW_ACTIVE_COUNT];
extern const u8 SkillExpansionPassiveIds[SKILL_EXPANSION_HERO_COUNT]
                                        [SKILL_EXPANSION_NEW_PASSIVE_COUNT];

// Per HeroId: the index of the hero's block in ActiveSkillIconAssets and ActiveSkillRequiredLevels.
extern const u8 SkillExpansionHeroSkillBlocks[SKILL_EXPANSION_HERO_COUNT];

static inline s8 *skillExpansion_passivePurchased(u8 playerIndex, u32 slot)
{
    if (slot < HERO_PASSIVE_SKILL_COUNT)
    {
        return &PLAYER(playerIndex).passiveSkillLevelsPurchased[slot];
    }
    return &skillExpansion_players[playerIndex]
                .passiveSkillLevelsPurchased[slot - HERO_PASSIVE_SKILL_COUNT];
}

static inline s8 *skillExpansion_passiveApplied(u8 playerIndex, u32 slot)
{
    if (slot < HERO_PASSIVE_SKILL_COUNT)
    {
        return &PLAYER(playerIndex).passiveSkillLevelsApplied[slot];
    }
    return &skillExpansion_players[playerIndex]
                .passiveSkillLevelsApplied[slot - HERO_PASSIVE_SKILL_COUNT];
}

static inline u8 *skillExpansion_activePurchased(u8 playerIndex, u32 row)
{
    if (row < ACTIVE_SKILL_NONE)
    {
        return &PLAYER(playerIndex).activeSkillLevelsPurchased[row];
    }
    return &skillExpansion_players[playerIndex].activeSkillLevelsPurchased[row - ACTIVE_SKILL_NONE];
}

static inline u8 *skillExpansion_activeLevel(u8 playerIndex, u32 row)
{
    if (row < ACTIVE_SKILL_NONE)
    {
        return &PLAYER(playerIndex).activeSkillLevels[row];
    }
    return &skillExpansion_players[playerIndex].activeSkillLevels[row - ACTIVE_SKILL_NONE];
}

// Stock curves cap applied levels at five, then apply one positive equipment bonus.
static inline u8 skillExpansion_effectiveActiveLevel(u8 playerIndex, u32 row)
{
    u8 level = *skillExpansion_activeLevel(playerIndex, row);
    if (level > ACTIVE_SKILL_MAX_LEVEL)
    {
        level = ACTIVE_SKILL_MAX_LEVEL;
    }
    if (level != 0 && PLAYER_STAT(playerIndex, STAT_ACTIVE_SKILL_LEVEL_BONUS) > 0)
    {
        level++;
    }
    return level;
}

static inline u8 *skillExpansion_activeCycleEnabled(u8 playerIndex, u32 row)
{
    if (row < ACTIVE_SKILL_NONE)
    {
        return &PLAYER(playerIndex).activeSkillCycleEnabled[row];
    }
    return &skillExpansion_players[playerIndex].activeSkillCycleEnabled[row - ACTIVE_SKILL_NONE];
}

/** The new row's definition, or NULL for stock rows and empty new rows. */
static inline const SkillExpansionActiveDef *skillExpansion_activeDef(u8 playerIndex, u32 row)
{
    if (row < SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW || row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT)
    {
        return NULL;
    }
    return skillExpansion_players[playerIndex]
        .activeDefs[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW];
}

/** Stock row whose pose, cooldown bit and cast requirements the row uses. */
static inline u32 skillExpansion_baseRow(u8 playerIndex, u32 row)
{
    const SkillExpansionActiveDef *def = skillExpansion_activeDef(playerIndex, row);

    return def != NULL ? def->baseRow : row;
}

// Skill icons share their palette bank with the tile or HUD panel. Colors 8..13 are the icon art
// ramp: green, red while the HUD can't cast the skill, grey on a cycle-disabled tree tile. Page 2
// skills reuse stock icons, so they get their own ramps (skill_expansion_hud.c).
#define SKILL_EXPANSION_ICON_ART_FIRST_COLOR 8
#define SKILL_EXPANSION_ICON_ART_COLOR_COUNT 6

// Icons made from user art carry their own colors for palette indices 8..15. Indices 0..7 stay the
// frame's colors.
#define SKILL_EXPANSION_ICON_OWN_COLOR_COUNT 8
// Icon palette groups per table. The tree shows each group in its own palette bank.
#define SKILL_EXPANSION_ICON_GROUP_COUNT 6

typedef struct SkillExpansionIconColors
{
    // The icon's tile asset.
    const u8 *icon;
    // BGR555 colors for palette indices 8..15, shared by every icon of the group.
    const u16 *colors;
    // Palette group, 0..SKILL_EXPANSION_ICON_GROUP_COUNT - 1.
    u8 group;
} SkillExpansionIconColors;

/** The colors of an icon made from user art, or NULL for icons that use the art ramps. */
const SkillExpansionIconColors *skillExpansion_iconColors(const void *icon);

extern const u16 SkillExpansionPage2ArtRamp[SKILL_EXPANSION_ICON_ART_COLOR_COUNT];
extern const u16 SkillExpansionPage2UncastableArtRamp[SKILL_EXPANSION_ICON_ART_COLOR_COUNT];
extern const u16 SkillExpansionPage2DisabledArtRamp[SKILL_EXPANSION_ICON_ART_COLOR_COUNT];

// Page 2 build a skill belongs to, shown as its icon color.
enum SkillExpansionBuild
{
    SKILL_EXPANSION_BUILD_NONE = 0,   // page 2 blue
    SKILL_EXPANSION_BUILD_GOOD = 1,   // gold
    SKILL_EXPANSION_BUILD_EVIL = 2,   // violet
    SKILL_EXPANSION_BUILD_TRICKS = 3, // green
    SKILL_EXPANSION_BUILD_DEAD = 4,   // pale ghost teal
    SKILL_EXPANSION_BUILD_GREY = 5,   // ash grey
    SKILL_EXPANSION_BUILD_FIRE = 6,   // ember red
};

u8 skillExpansion_activeBuild(u32 heroId, u32 row);
u8 skillExpansion_passiveBuild(u32 passiveSkillId);

/** Icon art ramp of a page 2 skill of `build` (SkillExpansionBuild). */
const u16 *skillExpansion_buildArtRamp(u8 build, bool32 uncastable);

const PassiveSkill *skillExpansion_getPassiveSkill(u32 passiveSkillId);
s32 skillExpansion_getPassiveRequiredLevel(u32 passiveSkillId, u32 purchasedLevel);
const void *skillExpansion_getPassiveIcon(u32 passiveSkillId);
const char *skillExpansion_getPassiveDescription(u32 passiveSkillId);
const char *skillExpansion_getStockActiveDescription(u32 heroId, u32 row);
u8 skillExpansion_findPassiveSlot(u8 playerIndex, u8 passiveSkillId);
s32 skillExpansion_getPassiveLevel(u8 playerIndex, u8 passiveSkillId);
void skillExpansion_passiveApplyAllLevels(u8 playerIndex);
void skillExpansion_addTemporaryLevels(u8 playerIndex, u8 count);
void skillExpansion_removeTemporaryLevels(u8 playerIndex, u8 count);
void skillExpansion_castStockCallback(Actor *playerActor);
u8 skillExpansion_getCastPose(u32 playerIndex, u32 row);
const SkillExpansionCastPolicy *skillExpansion_getCastPolicy(u8 playerIndex, u32 row);
u16 skillExpansion_activeId(u32 heroId, u32 row);
u16 skillExpansion_passiveId(u32 heroId, u32 slot);
u16 skillExpansion_legacyActiveId(u32 heroId, u32 row);
