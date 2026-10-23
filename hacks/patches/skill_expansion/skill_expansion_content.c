#include "patches/skill_expansion/aragorn_icons.h"
#include "patches/skill_expansion/frodo_icons.h"
#include "patches/skill_expansion/gandalf_icons.h"
#include "patches/skill_expansion/gimli_icons.h"
#include "patches/skill_expansion/legolas_icons.h"
#include "patches/skill_expansion/sam_icons.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_frodo.h"
#include "patches/skill_expansion/skill_expansion_gandalf.h"
#include "patches/skill_expansion/skill_expansion_gimli.h"
#include "patches/skill_expansion/skill_expansion_legolas.h"
#include "patches/skill_expansion/skill_expansion_sam.h"
#include "patches/skill_expansion/skill_expansion_smeagol.h"
#include "patches/skill_expansion/smeagol_icons.h"
#include "stats.h"

#if HACK_ringwraith
#include "patches/ringwraith/ringwraith.h"
#include "patches/ringwraith/ringwraith_icons.h"
#include "patches/ringwraith/ringwraith_kit.h"
#define RINGWRAITH_ICON(name) .icon = RingwraithIcon##name,
#else
#define RINGWRAITH_ICON(name)
#endif

// Cast fields of the stock rows the elite actives borrow from.
#define KINGS_COMMAND_CAST_FIELDS                                                                  \
    .field_0x20 = 2, .castFlags = {.d = {.forcePose0 = 1}}, .field_0x22 = 5,                       \
    .castTriggerFrame = 8, .field_0x24 = 0xff
#define NUMENOREAN_WILL_CAST_FIELDS                                                                \
    .field_0x20 = 5, .castFlags = {.d = {.forcePose0 = 1}}, .field_0x22 = 5,                       \
    .castTriggerFrame = 0, .field_0x24 = 0xff
#define FRODO_QUICK_CAST_FIELDS                                                                    \
    .field_0x20 = 2, .castFlags = {0}, .field_0x22 = 5, .castTriggerFrame = 2, .field_0x24 = 0xff
#define FRODO_RING_CAST_FIELDS                                                                     \
    .field_0x20 = 1, .castFlags = {.d = {.forcePose0 = 1}}, .field_0x22 = 5,                       \
    .castTriggerFrame = 0, .field_0x24 = 0xff
#define SMEAGOL_THROW_CAST_FIELDS                                                                  \
    .field_0x20 = 2, .castFlags = {0}, .field_0x22 = 5, .castTriggerFrame = 4, .field_0x24 = 0xff
#define SMEAGOL_COWER_CAST_FIELDS                                                                  \
    .field_0x20 = 2, .castFlags = {0}, .field_0x22 = 5, .castTriggerFrame = 0, .field_0x24 = 0xff
#define SAM_THROW_CAST_FIELDS                                                                      \
    .field_0x20 = 2, .castFlags = {0}, .field_0x22 = 5, .castTriggerFrame = 5, .field_0x24 = 0xff
#define SAM_PLANT_CAST_FIELDS                                                                      \
    .field_0x20 = 2, .castFlags = {0}, .field_0x22 = 5, .castTriggerFrame = 2, .field_0x24 = 0xff
#define SAM_SWING_CAST_FIELDS                                                                      \
    .field_0x20 = 2, .castFlags = {0}, .field_0x22 = 5, .castTriggerFrame = 0, .field_0x24 = 0xff

#define GIMLI_SWING_CAST_FIELDS                                                                    \
    .field_0x20 = 2, .castFlags = {0}, .field_0x22 = 5, .castTriggerFrame = 4, .field_0x24 = 0xff
#define GIMLI_SHOUT_CAST_FIELDS                                                                    \
    .field_0x20 = 2, .castFlags = {.d = {.forcePose0 = 1}}, .field_0x22 = 5,                       \
    .castTriggerFrame = 0, .field_0x24 = 0xff
#define GIMLI_BRACE_CAST_FIELDS                                                                    \
    .field_0x20 = 5, .castFlags = {.d = {.forcePose0 = 1}}, .field_0x22 = 5,                       \
    .castTriggerFrame = 0, .field_0x24 = 0xff

#define GANDALF_STAFF_CAST_FIELDS                                                                  \
    .field_0x20 = 2, .castFlags = {.d = {.forcePose0 = 1}}, .field_0x22 = 5,                       \
    .castTriggerFrame = 8, .field_0x24 = 0xff
#define LEGOLAS_NOCK_CAST_FIELDS                                                                   \
    .field_0x20 = 2, .castFlags = {.d = {.forcePose0 = 1}}, .field_0x22 = 5,                       \
    .castTriggerFrame = 0, .field_0x24 = 0xff
#define GANDALF_BUFF_CAST_FIELDS                                                                   \
    .field_0x20 = 2, .castFlags = {.d = {.forcePose0 = 1}}, .field_0x22 = 5,                       \
    .castTriggerFrame = 0, .field_0x24 = 0xff

// The borrowed stock rows set an action-state bit while their buff lasts. These policies keep the
// borrowed pose but not that bit, so an elite active never waits on the stock skill.
static const SkillExpansionCastPolicy KingsCommandPosePolicy = {
    ACTIVE_SKILL_ARAGORN_KINGS_COMMAND + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE,
    0};
static const SkillExpansionCastPolicy NumenoreanWillPosePolicy = {
    ACTIVE_SKILL_ARAGORN_NUMENOREAN_WILL + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE,
    0};
static const SkillExpansionCastPolicy CowerPosePolicy = {
    ACTIVE_SKILL_SMEAGOL_COWER + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionCastPolicy SnarePosePolicy = {
    ACTIVE_SKILL_SAM_SNARE + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionCastPolicy SamThrowPosePolicy = {
    ACTIVE_SKILL_SAM_KNIFE_TOSS + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionCastPolicy CookpotPosePolicy = {
    ACTIVE_SKILL_SAM_COOKPOT_SMASH + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionCastPolicy GimliSwingPosePolicy = {
    ACTIVE_SKILL_GIMLI_AXE_THROW + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionCastPolicy GimliCleavePosePolicy = {
    ACTIVE_SKILL_GIMLI_AXE_THROW + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, 4 * 30};
static const SkillExpansionCastPolicy GimliShoutPosePolicy = {
    ACTIVE_SKILL_GIMLI_DWARVEN_RAGE + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionCastPolicy GimliBracePosePolicy = {
    ACTIVE_SKILL_GIMLI_STOICISM + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, 0};

static const SkillExpansionCastPolicy LightstrikePosePolicy = {
    ACTIVE_SKILL_GANDALF_LIGHTSTRIKE + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionCastPolicy GandalfShieldPosePolicy = {
    ACTIVE_SKILL_GANDALF_SHIELD + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionCastPolicy BlindingAuraPosePolicy = {
    ACTIVE_SKILL_GANDALF_BLINDING_AURA + 1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE,
    0};
static const SkillExpansionCastPolicy FriendOfMirkwoodPosePolicy = {
    ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD + 1, SKILL_EXPANSION_RULE_NONE,
    SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionCastPolicy GwaihirPosePolicy = {ACTIVE_SKILL_GANDALF_SUMMON_GWAIHIR + 1,
                                                           SKILL_EXPANSION_RULE_NONE,
                                                           SKILL_EXPANSION_RULE_NONE, 0};

static const ActiveSkill AragornFlameOfTheWestRow = {
    .callback = skillExpansion_aragornFlameOfTheWest,
    .spiritCost = 60,
    NUMENOREAN_WILL_CAST_FIELDS,
};
static const s8 AragornFlameOfTheWestLevels[] = {12, 15, 18, 21, 24, -1};
static const SkillExpansionActiveDef AragornFlameOfTheWest = {
    .row = &AragornFlameOfTheWestRow,
    .icon = AragornIconFlameOfTheWest,
    .requiredLevels = AragornFlameOfTheWestLevels,
    .description = "Flame of the West (Level @1) - King. Spend the Flame bar: Anduril blazes 4s "
                   "+1s/level, +8s if full. Hits scorch foes 3 +3/level, x2 if full. @2 Spirit.",
    .canCast = skillExpansion_canCastFlameOfTheWest,
    .baseRow = ACTIVE_SKILL_ARAGORN_NUMENOREAN_WILL,
    .policy = &NumenoreanWillPosePolicy,
};

static const ActiveSkill AragornRangersAmbushRow = {
    .callback = skillExpansion_aragornRangersAmbush,
    .spiritCost = 45,
    KINGS_COMMAND_CAST_FIELDS,
};
static const s8 AragornRangersAmbushLevels[] = {10, 13, 16, 19, 22, -1};
static const SkillExpansionActiveDef AragornRangersAmbush = {
    .row = &AragornRangersAmbushRow,
    .icon = AragornIconRangersAmbush,
    .requiredLevels = AragornRangersAmbushLevels,
    .description = "Ranger's Ambush (Level @1) - Ranger. Unseen 2s +0.5s/level. Your next blow "
                   "crits for 150% +50%/level, stuns and stokes +20 Flame. @2 Spirit.",
    .canCast = skillExpansion_canCastRangersAmbush,
    .baseRow = ACTIVE_SKILL_ARAGORN_KINGS_COMMAND,
    .policy = &KingsCommandPosePolicy,
};

static const ActiveSkill AragornTrackersMarkRow = {
    .callback = skillExpansion_aragornTrackersMark,
    .spiritCost = 30,
    KINGS_COMMAND_CAST_FIELDS,
};
static const s8 AragornTrackersMarkLevels[] = {11, 14, 17, 20, 23, -1};
static const SkillExpansionActiveDef AragornTrackersMark = {
    .row = &AragornTrackersMarkRow,
    .icon = AragornIconTrackersMark,
    .requiredLevels = AragornTrackersMarkLevels,
    .description = "Tracker's Mark (Level @1) - Ranger. Mark the nearest foe 10s +2s/level: +15% "
                   "+5%/level damage, double Flame. Its death stokes +25. @2 Spirit.",
    .canCast = skillExpansion_canCastTrackersMark,
    .baseRow = ACTIVE_SKILL_ARAGORN_KINGS_COMMAND,
    .policy = &KingsCommandPosePolicy,
};

static const ActiveSkill AragornOathOfTheDeadRow = {
    .callback = skillExpansion_aragornOathOfTheDead,
    .spiritCost = 60,
    NUMENOREAN_WILL_CAST_FIELDS,
};
static const s8 AragornOathOfTheDeadLevels[] = {13, 16, 19, 22, 25, -1};
static const SkillExpansionActiveDef AragornOathOfTheDead = {
    .row = &AragornOathOfTheDeadRow,
    .icon = AragornIconOathOfTheDead,
    .requiredLevels = AragornOathOfTheDeadLevels,
    .description = "Oath of the Dead (Level @1) - Dead. Ghosts burst for 15 +8/level, stunning "
                   "foes and stoking +10 Flame each. No ghosts raises one. @2 Spirit.",
    .baseRow = ACTIVE_SKILL_ARAGORN_NUMENOREAN_WILL,
    .policy = &NumenoreanWillPosePolicy,
};

static const ActiveSkill AragornReturnOfTheKingRow = {
    .callback = skillExpansion_aragornReturnOfTheKing,
    .spiritCost = 150,
    NUMENOREAN_WILL_CAST_FIELDS,
};
static const s8 AragornReturnOfTheKingLevels[] = {16, 18, 20, 22, 25, -1};
static const SkillExpansionActiveDef AragornReturnOfTheKing = {
    .row = &AragornReturnOfTheKingRow,
    .icon = AragornIconReturnOfTheKing,
    .requiredLevels = AragornReturnOfTheKingLevels,
    .description = "Return of the King (Level @1) - King. Anduril is Reforged. All skills gain "
                   "+1 level and cost no Spirit for 10s +2s/level. @2 Spirit.",
    .canCast = skillExpansion_canCastReturnOfTheKing,
    .baseRow = ACTIVE_SKILL_ARAGORN_NUMENOREAN_WILL,
    .policy = &NumenoreanWillPosePolicy,
};

static const ActiveSkill FrodoPhialOfGaladrielRow = {
    .callback = skillExpansion_frodoPhialOfGaladriel,
    .spiritCost = 40,
    FRODO_QUICK_CAST_FIELDS,
};
static const s8 FrodoPhialOfGaladrielLevels[] = {10, 13, 16, 19, 22, -1};
static const SkillExpansionActiveDef FrodoPhialOfGaladriel = {
    .row = &FrodoPhialOfGaladrielRow,
    .icon = FrodoIconPhialOfGaladriel,
    .requiredLevels = FrodoPhialOfGaladrielLevels,
    .description = "Phial of Galadriel (Level @1) - Good. Burn off all Burden in a stunning "
                   "blast, bigger with more. The light sears foes 6s +1s/level. @2 Spirit.",
    .canCast = skillExpansion_canCastPhialOfGaladriel,
    .baseRow = ACTIVE_SKILL_FRODO_KNIFE_TOSS,
};

static const ActiveSkill FrodoElberethRow = {
    .callback = skillExpansion_frodoElbereth,
    .spiritCost = 45,
    FRODO_QUICK_CAST_FIELDS,
};
static const s8 FrodoElberethLevels[] = {12, 15, 18, 21, 24, -1};
static const SkillExpansionActiveDef FrodoElbereth = {
    .row = &FrodoElberethRow,
    .icon = FrodoIconElbereth,
    .requiredLevels = FrodoElberethLevels,
    .description = "Elbereth! (Level @1) - Good. Starlight stuns nearby foes for 2s +0.5s/level, "
                   "heals you and your ally 10% +3%/level and removes 20 Burden. @2 Spirit.",
    .baseRow = ACTIVE_SKILL_FRODO_KNIFE_TOSS,
};

static const ActiveSkill FrodoShadowOfTheRingRow = {
    .callback = skillExpansion_frodoShadowOfTheRing,
    .spiritCost = 30,
    FRODO_RING_CAST_FIELDS,
};
static const s8 FrodoShadowOfTheRingLevels[] = {11, 14, 17, 20, 23, -1};
static const SkillExpansionActiveDef FrodoShadowOfTheRing = {
    .row = &FrodoShadowOfTheRingRow,
    .icon = FrodoIconShadowOfTheRing,
    .requiredLevels = FrodoShadowOfTheRingLevels,
    .description = "Shadow of the Ring (Level @1) - Evil. A wave of shadow hits foes in front "
                   "for 12, plus 0.3 +0.1/level per Burden. Adds 15 Burden. @2 Spirit.",
    .baseRow = ACTIVE_SKILL_FRODO_KNIFE_TOSS,
};

static const ActiveSkill FrodoSowDiscordRow = {
    .spiritCost = 35,
    FRODO_RING_CAST_FIELDS,
};
static const s8 FrodoSowDiscordLevels[] = {13, 16, 19, 22, 25, -1};
static const SkillExpansionActiveDef FrodoSowDiscord = {
    .row = &FrodoSowDiscordRow,
    .icon = FrodoIconSowDiscord,
    .requiredLevels = FrodoSowDiscordLevels,
    .description = "Sow Discord (Level @1) - Tricks. The nearest 2 +1/level foes turn on each "
                   "other for 5s +1s/level. @2 Spirit.",
    .resolve = skillExpansion_frodoSowDiscord,
    .baseRow = ACTIVE_SKILL_FRODO_KNIFE_TOSS,
};

static const ActiveSkill FrodoSmokeAndMirrorsRow = {
    .spiritCost = 25,
    FRODO_QUICK_CAST_FIELDS,
};
static const s8 FrodoSmokeAndMirrorsLevels[] = {14, 17, 20, 23, 25, -1};
static const SkillExpansionActiveDef FrodoSmokeAndMirrors = {
    .row = &FrodoSmokeAndMirrorsRow,
    .icon = FrodoIconSmokeAndMirrors,
    .requiredLevels = FrodoSmokeAndMirrorsLevels,
    .description = "Smoke and Mirrors (Level @1) - Tricks. Slip away backward, leaving your "
                   "cloak behind. Foes near it are fooled for 2s +0.5s/level. @2 Spirit.",
    .resolve = skillExpansion_frodoSmokeAndMirrors,
    .baseRow = ACTIVE_SKILL_FRODO_KNIFE_TOSS,
};

static const ActiveSkill FrodoPutOnTheRingRow = {
    .spiritCost = 20,
    FRODO_RING_CAST_FIELDS,
};
static const s8 FrodoPutOnTheRingLevels[] = {15, 18, 21, 24, 25, -1};
static const SkillExpansionActiveDef FrodoPutOnTheRing = {
    .row = &FrodoPutOnTheRingRow,
    .icon = FrodoIconPutOnTheRing,
    .requiredLevels = FrodoPutOnTheRingLevels,
    .description = "Put On the Ring (Level @1) - Evil. Vanish until you attack or cast it again. "
                   "Burden climbs, slower per level. Shadow of the Ring is free. @2 Spirit.",
    .resolve = skillExpansion_frodoPutOnTheRing,
    .baseRow = ACTIVE_SKILL_FRODO_KNIFE_TOSS,
};

static const ActiveSkill SmeagolPlayDeadRow = {
    .callback = skillExpansion_smeagolPlayDead,
    .spiritCost = 30,
    SMEAGOL_COWER_CAST_FIELDS,
};
static const s8 SmeagolPlayDeadLevels[] = {10, 13, 16, 19, 22, -1};
static const SkillExpansionActiveDef SmeagolPlayDead = {
    .row = &SmeagolPlayDeadRow,
    .icon = SmeagolIconPlayDead,
    .requiredLevels = SmeagolPlayDeadLevels,
    .description = "Play Dead (Level @1) - Smeagol. Drop flat and unseen 2s +0.5s/level, easing "
                   "the grip 10. Press B to get up: your next hit deals x3. @2 Spirit.",
    .canCast = skillExpansion_canCastSmeagolSkill,
    .baseRow = ACTIVE_SKILL_SMEAGOL_COWER,
    .policy = &CowerPosePolicy,
};

static const ActiveSkill SmeagolThrottleRow = {
    .spiritCost = 40,
    SMEAGOL_THROW_CAST_FIELDS,
};
static const s8 SmeagolThrottleLevels[] = {11, 14, 17, 20, 23, -1};
static const SkillExpansionActiveDef SmeagolThrottle = {
    .row = &SmeagolThrottleRow,
    .icon = SmeagolIconThrottle,
    .requiredLevels = SmeagolThrottleLevels,
    .description = "Throttle (Level @1) - Gollum. Ride a foe's back 3s +0.5s/level. Steer it into "
                   "foes to ram them for 6 +3/level while it chokes. +10 grip. @2 Spirit.",
    .canCast = skillExpansion_canCastSmeagolSkill,
    .resolve = skillExpansion_smeagolThrottle,
    .baseRow = ACTIVE_SKILL_SMEAGOL_ROCK_THROW,
};

static const ActiveSkill SmeagolDropThePreciousRow = {
    .callback = skillExpansion_smeagolDropThePrecious,
    .spiritCost = 45,
    SMEAGOL_COWER_CAST_FIELDS,
};
static const s8 SmeagolDropThePreciousLevels[] = {12, 15, 18, 21, 24, -1};
static const SkillExpansionActiveDef SmeagolDropThePrecious = {
    .row = &SmeagolDropThePreciousRow,
    .icon = SmeagolIconDropThePrecious,
    .requiredLevels = SmeagolDropThePreciousLevels,
    .description = "Drop the Precious (Level @1) - Precious. Foes crowd the Ring. Grab it back in "
                   "8s for a burst of 15 +6/level, or Gollum takes over. @2 Spirit.",
    .canCast = skillExpansion_canCastSmeagolSkill,
    .baseRow = ACTIVE_SKILL_SMEAGOL_COWER,
    .policy = &CowerPosePolicy,
};

static const ActiveSkill SmeagolCatchAFishRow = {
    .callback = skillExpansion_smeagolCatchAFish,
    .spiritCost = 20,
    SMEAGOL_THROW_CAST_FIELDS,
};
static const s8 SmeagolCatchAFishLevels[] = {13, 16, 19, 22, 25, -1};
static const SkillExpansionActiveDef SmeagolCatchAFish = {
    .row = &SmeagolCatchAFishRow,
    .icon = SmeagolIconCatchAFish,
    .requiredLevels = SmeagolCatchAFishLevels,
    .description = "Catch a Fish (Level @1) - Smeagol. For 1.5s, snatch arrows flying at you and "
                   "throw them back for x2. Catches 1 +1 per 2 levels. @2 Spirit.",
    .canCast = skillExpansion_canCastSmeagolSkill,
    .baseRow = ACTIVE_SKILL_SMEAGOL_ROCK_THROW,
};

static const ActiveSkill SmeagolSlinkRow = {
    .spiritCost = 25,
    SMEAGOL_THROW_CAST_FIELDS,
};
static const s8 SmeagolSlinkLevels[] = {14, 17, 20, 23, 25, -1};
static const SkillExpansionActiveDef SmeagolSlink = {
    .row = &SmeagolSlinkRow,
    .icon = SmeagolIconSlink,
    .requiredLevels = SmeagolSlinkLevels,
    .description = "Slink (Level @1) - Gollum. Crawl ahead through walls and over pits, then "
                   "hide 1s +0.25s/level. +5 grip. @2 Spirit.",
    .canCast = skillExpansion_canCastSmeagolSkill,
    .resolve = skillExpansion_smeagolSlink,
    .baseRow = ACTIVE_SKILL_SMEAGOL_ROCK_THROW,
};

static const ActiveSkill SmeagolArgueWithYourselfRow = {
    .callback = skillExpansion_smeagolArgueWithYourself,
    .spiritCost = 15,
    SMEAGOL_COWER_CAST_FIELDS,
};
static const s8 SmeagolArgueWithYourselfLevels[] = {15, 18, 21, 24, 25, -1};
static const SkillExpansionActiveDef SmeagolArgueWithYourself = {
    .row = &SmeagolArgueWithYourselfRow,
    .icon = SmeagolIconArgueWithYourself,
    .requiredLevels = SmeagolArgueWithYourselfLevels,
    .description = "Argue With Yourself (Level @1) - Precious. Ease the grip 25 +5/level. At 60+ "
                   "grip the shouting stuns nearby foes 1s +0.25s/level. @2 Spirit.",
    .canCast = skillExpansion_canCastSmeagolSkill,
    .baseRow = ACTIVE_SKILL_SMEAGOL_COWER,
    .policy = &CowerPosePolicy,
};

static const ActiveSkill SamPlantTatersRow = {
    .callback = skillExpansion_samPlantTaters,
    .spiritCost = 25,
    SAM_PLANT_CAST_FIELDS,
};
static const s8 SamPlantTatersLevels[] = {10, 13, 16, 19, 22, -1};
static const SkillExpansionActiveDef SamPlantTaters = {
    .row = &SamPlantTatersRow,
    .icon = SamIconPlantTaters,
    .requiredLevels = SamPlantTatersLevels,
    .description = "Plant Taters (Level @1) - Gardener. Ripe taters pelt and stun foes near them "
                   "for 6 +3/level. Walk over one to harvest Spirit. @2 Spirit.",
    .baseRow = ACTIVE_SKILL_SAM_SNARE,
    .policy = &SnarePosePolicy,
};

static const ActiveSkill SamElvenRopeRow = {
    .spiritCost = 30,
    SAM_THROW_CAST_FIELDS,
};
static const s8 SamElvenRopeLevels[] = {11, 14, 17, 20, 23, -1};
static const SkillExpansionActiveDef SamElvenRope = {
    .row = &SamElvenRopeRow,
    .icon = SamIconElvenRope,
    .requiredLevels = SamElvenRopeLevels,
    .description = "Elven Rope (Level @1) - Stout. Lasso the nearest foe, pull it to you and stun "
                   "it 1.5s +0.25s/level. @2 Spirit.",
    .resolve = skillExpansion_samElvenRope,
    .baseRow = ACTIVE_SKILL_SAM_KNIFE_TOSS,
    .policy = &SamThrowPosePolicy,
};

static const ActiveSkill SamSecondBreakfastRow = {
    .callback = skillExpansion_samSecondBreakfast,
    .spiritCost = 40,
    SAM_SWING_CAST_FIELDS,
};
static const s8 SamSecondBreakfastLevels[] = {12, 15, 18, 21, 24, -1};
static const SkillExpansionActiveDef SamSecondBreakfast = {
    .row = &SamSecondBreakfastRow,
    .icon = SamIconSecondBreakfast,
    .requiredLevels = SamSecondBreakfastLevels,
    .description = "Second Breakfast (Level @1) - Cook. Set a stew pot for 8s +1s/level. Heroes "
                   "near it heal 2% +1%/level of max HP each second. @2 Spirit.",
    .baseRow = ACTIVE_SKILL_SAM_COOKPOT_SMASH,
    .policy = &CookpotPosePolicy,
};

static const ActiveSkill SamBrambleHedgeRow = {
    .callback = skillExpansion_samBrambleHedge,
    .spiritCost = 35,
    SAM_PLANT_CAST_FIELDS,
};
static const s8 SamBrambleHedgeLevels[] = {13, 16, 19, 22, 25, -1};
static const SkillExpansionActiveDef SamBrambleHedge = {
    .row = &SamBrambleHedgeRow,
    .icon = SamIconBrambleHedge,
    .requiredLevels = SamBrambleHedgeLevels,
    .description = "Bramble Hedge (Level @1) - Gardener. Thorns snag foes each second for 3 "
                   "+2/level, spreading wider as they grow. @2 Spirit.",
    .baseRow = ACTIVE_SKILL_SAM_SNARE,
    .policy = &SnarePosePolicy,
};

static const ActiveSkill SamFryingPanParryRow = {
    .callback = skillExpansion_samFryingPanParry,
    .spiritCost = 25,
    SAM_SWING_CAST_FIELDS,
};
static const s8 SamFryingPanParryLevels[] = {14, 17, 20, 23, 25, -1};
static const SkillExpansionActiveDef SamFryingPanParry = {
    .row = &SamFryingPanParryRow,
    .icon = SamIconFryingPanParry,
    .requiredLevels = SamFryingPanParryLevels,
    .description = "Frying Pan Parry (Level @1) - Stout. Block every hit for 2s +0.2s/level. Then "
                   "bonk nearby foes for 6 +3/level per block. @2 Spirit.",
    .canCast = skillExpansion_canCastSamParry,
    .baseRow = ACTIVE_SKILL_SAM_COOKPOT_SMASH,
    .policy = &CookpotPosePolicy,
};

static const ActiveSkill SamPoTayToesRow = {
    .spiritCost = 30,
    SAM_THROW_CAST_FIELDS,
};
static const s8 SamPoTayToesLevels[] = {15, 18, 21, 24, 25, -1};
static const SkillExpansionActiveDef SamPoTayToes = {
    .row = &SamPoTayToesRow,
    .icon = SamIconPoTayToes,
    .requiredLevels = SamPoTayToesLevels,
    .description = "Po-tay-toes! (Level @1) - Cook. Throw all your taters for 12 +6/level, less "
                   "if unripe. Ripe ones stun. @2 Spirit.",
    .resolve = skillExpansion_samPoTayToes,
    .baseRow = ACTIVE_SKILL_SAM_KNIFE_TOSS,
    .policy = &SamThrowPosePolicy,
};

// Gimli's page 2 requiredLevels count in GIMLI_TALLY_KILLS_PER_STEP kills.
static const ActiveSkill GimliBarukKhazadRow = {
    .spiritCost = 35,
    GIMLI_SHOUT_CAST_FIELDS,
};
static const s8 GimliBarukKhazadLevels[] = {1, 3, 5, 8, 12, -1};
static const SkillExpansionActiveDef GimliBarukKhazad = {
    .row = &GimliBarukKhazadRow,
    .icon = GimliIconBarukKhazad,
    .requiredLevels = GimliBarukKhazadLevels,
    .description = "Baruk Khazad! (Kills @1) - Stone. Stun nearby foes 1s +0.2s/level. Each one "
                   "stunned cuts your damage taken 4% for 4s +1s/level. @2 Spirit.",
    .resolve = skillExpansion_gimliBarukKhazad,
    .baseRow = ACTIVE_SKILL_GIMLI_DWARVEN_RAGE,
    .policy = &GimliShoutPosePolicy,
};

static const ActiveSkill GimliCleaveTheWeakRow = {
    .spiritCost = 30,
    GIMLI_SWING_CAST_FIELDS,
};
static const s8 GimliCleaveTheWeakLevels[] = {1, 3, 6, 9, 13, -1};
static const SkillExpansionActiveDef GimliCleaveTheWeak = {
    .row = &GimliCleaveTheWeakRow,
    .icon = GimliIconCleaveTheWeak,
    .requiredLevels = GimliCleaveTheWeakLevels,
    .description = "Cleave the Weak (Kills @1) - Fury. Hit the nearest foe for 8 +4/level. Below "
                   "15% +3%/level HP it dies, refunding Spirit and cooldown. @2 Spirit.",
    .resolve = skillExpansion_gimliCleaveTheWeak,
    .baseRow = ACTIVE_SKILL_GIMLI_AXE_THROW,
    .policy = &GimliCleavePosePolicy,
};

static const ActiveSkill GimliNobodyTossesADwarfRow = {
    .spiritCost = 40,
    GIMLI_SWING_CAST_FIELDS,
};
static const s8 GimliNobodyTossesADwarfLevels[] = {2, 4, 7, 10, 14, -1};
static const SkillExpansionActiveDef GimliNobodyTossesADwarf = {
    .row = &GimliNobodyTossesADwarfRow,
    .icon = GimliIconNobodyTossesADwarf,
    .requiredLevels = GimliNobodyTossesADwarfLevels,
    .description = "Nobody Tosses a Dwarf (Kills @1) - Tally. Leap to the nearest foe and slam "
                   "for 10 +5/level. Slam kills add 2 to the Tally. @2 Spirit.",
    .resolve = skillExpansion_gimliNobodyTossesADwarf,
    .baseRow = ACTIVE_SKILL_GIMLI_AXE_THROW,
    .policy = &GimliSwingPosePolicy,
};

static const ActiveSkill GimliHoldTheDeepRow = {
    .callback = skillExpansion_gimliHoldTheDeep,
    .spiritCost = 45,
    GIMLI_BRACE_CAST_FIELDS,
};
static const s8 GimliHoldTheDeepLevels[] = {4, 7, 10, 14, 18, -1};
static const SkillExpansionActiveDef GimliHoldTheDeep = {
    .row = &GimliHoldTheDeepRow,
    .icon = GimliIconHoldTheDeep,
    .requiredLevels = GimliHoldTheDeepLevels,
    .description = "Hold the Deep (Kills @1) - Stone. Stand firm 3s taking half damage, then "
                   "quake for 10 +4/level plus 60% +10%/level of it. @2 Spirit.",
    .canCast = skillExpansion_canCastGimliHold,
    .baseRow = ACTIVE_SKILL_GIMLI_STOICISM,
    .policy = &GimliBracePosePolicy,
};

static const ActiveSkill GimliBookOfGrudgesRow = {
    .spiritCost = 20,
    GIMLI_SWING_CAST_FIELDS,
};
static const s8 GimliBookOfGrudgesLevels[] = {5, 8, 11, 15, 20, -1};
static const SkillExpansionActiveDef GimliBookOfGrudges = {
    .row = &GimliBookOfGrudgesRow,
    .icon = GimliIconBookOfGrudges,
    .requiredLevels = GimliBookOfGrudgesLevels,
    .description = "Book of Grudges (Kills @1) - Tally. Mark the nearest foe's kind for this area: "
                   "+10%/level damage to them, kills add 2 to the Tally. @2 Spirit.",
    .resolve = skillExpansion_gimliBookOfGrudges,
    .baseRow = ACTIVE_SKILL_GIMLI_AXE_THROW,
    .policy = &GimliSwingPosePolicy,
};

static const ActiveSkill GimliStillCountingRow = {
    .callback = skillExpansion_gimliStillCounting,
    .spiritCost = 80,
    GIMLI_SHOUT_CAST_FIELDS,
};
static const s8 GimliStillCountingLevels[] = {15, 22, 30, 40, 50, -1};
static const SkillExpansionActiveDef GimliStillCounting = {
    .row = &GimliStillCountingRow,
    .icon = GimliIconStillCounting,
    .requiredLevels = GimliStillCountingLevels,
    .description = "Still Counting (Kills @1) - Fury. For 12s +2s/level, each kill heals 3% and "
                   "adds 1s. Ends in a blast of 4 per kill. @2 Spirit.",
    .canCast = skillExpansion_canCastGimliCounting,
    .baseRow = ACTIVE_SKILL_GIMLI_DWARVEN_RAGE,
    .policy = &GimliShoutPosePolicy,
};

// Legolas's rows nock an arrow that fires on the next loose of his Overdraw.
static const ActiveSkill LegolasPiercingShotRow = {
    .spiritCost = 20,
    LEGOLAS_NOCK_CAST_FIELDS,
};
static const s8 LegolasPiercingShotLevels[] = {10, 13, 16, 19, 22, -1};
static const SkillExpansionActiveDef LegolasPiercingShot = {
    .row = &LegolasPiercingShotRow,
    .icon = LegolasIconPiercingShot,
    .requiredLevels = LegolasPiercingShotLevels,
    .description = "Piercing Shot (Level @1) - Nock: the loose pierces every foe on a long line for "
                   "14 +6/level. True: +50%. @2 Spirit.",
    .resolve = skillExpansion_legolasNock,
    .canCast = skillExpansion_canCastLegolasNock,
    .baseRow = ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD,
    .policy = &FriendOfMirkwoodPosePolicy,
};

static const ActiveSkill LegolasShieldSurfRow = {
    .spiritCost = 15,
    LEGOLAS_NOCK_CAST_FIELDS,
};
static const s8 LegolasShieldSurfLevels[] = {11, 14, 17, 20, 23, -1};
static const SkillExpansionActiveDef LegolasShieldSurf = {
    .row = &LegolasShieldSurfRow,
    .icon = LegolasIconShieldSurf,
    .requiredLevels = LegolasShieldSurfLevels,
    .description = "Shield-Surf (Level @1) - Nock: the loose slides you back 96px. True: you cannot be "
                   "hit or seen for 1s. @2 Spirit.",
    .resolve = skillExpansion_legolasNock,
    .canCast = skillExpansion_canCastLegolasNock,
    .baseRow = ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD,
    .policy = &FriendOfMirkwoodPosePolicy,
};

static const ActiveSkill LegolasPinningShotRow = {
    .spiritCost = 20,
    LEGOLAS_NOCK_CAST_FIELDS,
};
static const s8 LegolasPinningShotLevels[] = {12, 15, 18, 21, 24, -1};
static const SkillExpansionActiveDef LegolasPinningShot = {
    .row = &LegolasPinningShotRow,
    .icon = LegolasIconPinningShot,
    .requiredLevels = LegolasPinningShotLevels,
    .description = "Pinning Shot (Level @1) - Nock: pins the first foe on the line 1.5s +0.3s/level. "
                   "True: foes near it too. @2 Spirit.",
    .resolve = skillExpansion_legolasNock,
    .canCast = skillExpansion_canCastLegolasNock,
    .baseRow = ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD,
    .policy = &FriendOfMirkwoodPosePolicy,
};

static const ActiveSkill LegolasGiftRow = {
    .spiritCost = 15,
    LEGOLAS_NOCK_CAST_FIELDS,
};
static const s8 LegolasGiftLevels[] = {13, 16, 19, 22, 25, -1};
static const SkillExpansionActiveDef LegolasGift = {
    .row = &LegolasGiftRow,
    .icon = LegolasIconGiftOfTheGaladhrim,
    .requiredLevels = LegolasGiftLevels,
    .description = "Gift of the Galadhrim (Level @1) - Nock: 4 +2/level Spirit per foe on the line. "
                   "True: refunds the nock. @2 Spirit.",
    .resolve = skillExpansion_legolasNock,
    .canCast = skillExpansion_canCastLegolasNock,
    .baseRow = ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD,
    .policy = &FriendOfMirkwoodPosePolicy,
};

static const ActiveSkill LegolasVolleyRow = {
    .spiritCost = 35,
    LEGOLAS_NOCK_CAST_FIELDS,
};
static const s8 LegolasVolleyLevels[] = {14, 17, 20, 23, 25, -1};
static const SkillExpansionActiveDef LegolasVolley = {
    .row = &LegolasVolleyRow,
    .icon = LegolasIconVolley,
    .requiredLevels = LegolasVolleyLevels,
    .description = "Volley of Helm's Deep (Level @1) - Nock: 1s later 3 waves of 6 +3/level rain "
                   "ahead. True: 5 waves. @2 Spirit.",
    .resolve = skillExpansion_legolasNock,
    .canCast = skillExpansion_canCastLegolasNock,
    .baseRow = ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD,
    .policy = &FriendOfMirkwoodPosePolicy,
};

static const ActiveSkill LegolasOnlyCountsAsOneRow = {
    .spiritCost = 50,
    LEGOLAS_NOCK_CAST_FIELDS,
};
static const s8 LegolasOnlyCountsAsOneLevels[] = {18, 20, 22, 24, 25, -1};
static const SkillExpansionActiveDef LegolasOnlyCountsAsOne = {
    .row = &LegolasOnlyCountsAsOneRow,
    .icon = LegolasIconOnlyCountsAsOne,
    .requiredLevels = LegolasOnlyCountsAsOneLevels,
    .description = "Still Only Counts as One! (Level @1) - Nock: 30 +10/level, +50% per streak, "
                   "spends it. True: counts itself. @2 Spirit.",
    .resolve = skillExpansion_legolasNock,
    .canCast = skillExpansion_canCastLegolasNock,
    .baseRow = ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD,
    .policy = &FriendOfMirkwoodPosePolicy,
};

// Each Gandalf row casts its Grey or its White half by his form.
static const ActiveSkill GandalfFireworksRow = {
    .spiritCost = 15,
    GANDALF_STAFF_CAST_FIELDS,
};
static const s8 GandalfFireworksLevels[] = {10, 13, 16, 19, 22, -1};
static const SkillExpansionActiveDef GandalfFireworks = {
    .row = &GandalfFireworksRow,
    .icon = GandalfIconFireworks,
    .requiredLevels = GandalfFireworksLevels,
    .description = "Fireworks (Level @1) - Grey: a stunning burst ahead, then 3 pops. White: Flame "
                   "of Anor, a long beam for 20 +8/level. @2 Spirit.",
    .resolve = skillExpansion_gandalfFireworks,
    .canCast = skillExpansion_canCastGandalfSkill,
    .baseRow = ACTIVE_SKILL_GANDALF_LIGHTSTRIKE,
    .policy = &LightstrikePosePolicy,
};

static const ActiveSkill GandalfNeverLateRow = {
    .spiritCost = 15,
    GANDALF_BUFF_CAST_FIELDS,
};
static const s8 GandalfNeverLateLevels[] = {11, 14, 17, 20, 23, -1};
static const SkillExpansionActiveDef GandalfNeverLate = {
    .row = &GandalfNeverLateRow,
    .icon = GandalfIconNeverLate,
    .requiredLevels = GandalfNeverLateLevels,
    .description = "A Wizard Is Never Late (Level @1) - Grey: in 4s return here with this HP and "
                   "Spirit. White: Shadowfax, charge through foes. @2 Spirit.",
    .resolve = skillExpansion_gandalfNeverLate,
    .canCast = skillExpansion_canCastGandalfNeverLate,
    .baseRow = ACTIVE_SKILL_GANDALF_SHIELD,
    .policy = &GandalfShieldPosePolicy,
};

static const ActiveSkill GandalfSmokeRingsRow = {
    .spiritCost = 10,
    GANDALF_STAFF_CAST_FIELDS,
};
static const s8 GandalfSmokeRingsLevels[] = {12, 15, 18, 21, 24, -1};
static const SkillExpansionActiveDef GandalfSmokeRings = {
    .row = &GandalfSmokeRingsRow,
    .icon = GandalfIconSmokeRings,
    .requiredLevels = GandalfSmokeRingsLevels,
    .description = "Smoke Rings (Level @1) - Grey: a stunning cloud, White spells +10%/level in "
                   "it. White: break your staff, foes near take more, deal half. @2 Spirit.",
    .resolve = skillExpansion_gandalfSmokeRings,
    .canCast = skillExpansion_canCastGandalfSkill,
    .baseRow = ACTIVE_SKILL_GANDALF_LIGHTSTRIKE,
    .policy = &LightstrikePosePolicy,
};

static const ActiveSkill GandalfPipeWeedRow = {
    .spiritCost = 0,
    GANDALF_BUFF_CAST_FIELDS,
};
static const s8 GandalfPipeWeedLevels[] = {13, 16, 19, 22, 25, -1};
static const SkillExpansionActiveDef GandalfPipeWeed = {
    .row = &GandalfPipeWeedRow,
    .icon = GandalfIconPipeWeed,
    .requiredLevels = GandalfPipeWeedLevels,
    .description = "Pipe-weed Pause (Level @1) - Grey: smoke 3s unseen for 8 +2/level Spirit/s, "
                   "any input ends it. White: heal you and an ally 20% +4%/level. @2 Spirit.",
    .resolve = skillExpansion_gandalfPipeWeed,
    .canCast = skillExpansion_canCastGandalfPipeWeed,
    .baseRow = ACTIVE_SKILL_GANDALF_BLINDING_AURA,
    .policy = &BlindingAuraPosePolicy,
};

static const ActiveSkill GandalfFlyYouFoolsRow = {
    .spiritCost = 20,
    GANDALF_BUFF_CAST_FIELDS,
};
static const s8 GandalfFlyYouFoolsLevels[] = {14, 17, 20, 23, 25, -1};
static const SkillExpansionActiveDef GandalfFlyYouFools = {
    .row = &GandalfFlyYouFoolsRow,
    .icon = GandalfIconFlyYouFools,
    .requiredLevels = GandalfFlyYouFoolsLevels,
    .description = "Fly, You Fools! (Level @1) - Grey: shove foes off and run 50% faster 3s. "
                   "White: Look to My Coming, a dawn blast that stuns 2s. @2 Spirit.",
    .resolve = skillExpansion_gandalfFlyYouFools,
    .canCast = skillExpansion_canCastGandalfSkill,
    .baseRow = ACTIVE_SKILL_GANDALF_BLINDING_AURA,
    .policy = &BlindingAuraPosePolicy,
};

static const ActiveSkill GandalfShallNotPassRow = {
    .spiritCost = 70,
    GANDALF_BUFF_CAST_FIELDS,
};
static const s8 GandalfShallNotPassLevels[] = {18, 20, 22, 24, 25, -1};
static const SkillExpansionActiveDef GandalfShallNotPass = {
    .row = &GandalfShallNotPassRow,
    .icon = GandalfIconYouShallNotPass,
    .requiredLevels = GandalfShallNotPassLevels,
    .description = "You Shall Not Pass! (Level @1) - Grey: hold the line 3s, then blast and fall. "
                   "White: Light of Valinor, all your Spirit in one blast. @2 Spirit.",
    .resolve = skillExpansion_gandalfShallNotPass,
    .canCast = skillExpansion_canCastGandalfSkill,
    .baseRow = ACTIVE_SKILL_GANDALF_SUMMON_GWAIHIR,
    .policy = &GwaihirPosePolicy,
};

const SkillExpansionActiveDef *const SkillExpansionActiveDefs[SKILL_EXPANSION_HERO_COUNT]
                                                             [SKILL_EXPANSION_NEW_ACTIVE_COUNT] = {
                                                                 [HERO_ID_ARAGORN] =
                                                                     {
                                                                         &AragornRangersAmbush,
                                                                         &AragornOathOfTheDead,
                                                                         &AragornFlameOfTheWest,
                                                                         &AragornTrackersMark,
                                                                         &AragornReturnOfTheKing,
                                                                     },
                                                                 [HERO_ID_FRODO] =
                                                                     {
                                                                         &FrodoPhialOfGaladriel,
                                                                         &FrodoShadowOfTheRing,
                                                                         &FrodoSowDiscord,
                                                                         &FrodoElbereth,
                                                                         &FrodoSmokeAndMirrors,
                                                                         &FrodoPutOnTheRing,
                                                                     },
                                                                 [HERO_ID_SMEAGOL] =
                                                                     {
                                                                         &SmeagolPlayDead,
                                                                         &SmeagolThrottle,
                                                                         &SmeagolDropThePrecious,
                                                                         &SmeagolCatchAFish,
                                                                         &SmeagolSlink,
                                                                         &SmeagolArgueWithYourself,
                                                                     },
                                                                 [HERO_ID_SAM] =
                                                                     {
                                                                         &SamPlantTaters,
                                                                         &SamElvenRope,
                                                                         &SamSecondBreakfast,
                                                                         &SamBrambleHedge,
                                                                         &SamFryingPanParry,
                                                                         &SamPoTayToes,
                                                                     },
                                                                 [HERO_ID_LEGOLAS] =
                                                                     {
                                                                         &LegolasPiercingShot,
                                                                         &LegolasShieldSurf,
                                                                         &LegolasPinningShot,
                                                                         &LegolasGift,
                                                                         &LegolasVolley,
                                                                         &LegolasOnlyCountsAsOne,
                                                                     },
                                                                 [HERO_ID_GANDALF] =
                                                                     {
                                                                         &GandalfFireworks,
                                                                         &GandalfNeverLate,
                                                                         &GandalfSmokeRings,
                                                                         &GandalfPipeWeed,
                                                                         &GandalfFlyYouFools,
                                                                         &GandalfShallNotPass,
                                                                     },
                                                                 [HERO_ID_GIMLI] =
                                                                     {
                                                                         &GimliBarukKhazad,
                                                                         &GimliCleaveTheWeak,
                                                                         &GimliNobodyTossesADwarf,
                                                                         &GimliHoldTheDeep,
                                                                         &GimliBookOfGrudges,
                                                                         &GimliStillCounting,
                                                                     },

};

// Page 1 descriptions for stock rows whose casts these patches change. NULL keeps the stock text.
static const char *const AragornStockDescriptions[HERO_ACTIVE_SKILL_COUNT] = {
    [ACTIVE_SKILL_ARAGORN_SWEEP] =
        "Sweep (Level @1) - Strike all foes around you for +2 "
        "damage/level. Stokes +8 Flame, more for each foe hit. @2 Spirit.",
    [ACTIVE_SKILL_ARAGORN_KINGS_COMMAND] = "King's Command (Level @1) - Stun all foes around you "
                                           "for 3s +0.5s/level. Stokes +10 Flame. @2 Spirit.",
    [ACTIVE_SKILL_ARAGORN_SWORD_THROW] =
        "Sword Throw (Level @1) - Throw a sword for 10 +2/level, homing from level 3. Hits stoke "
        "Flame, and scorch during Flame of the West. @2 Spirit.",
    [ACTIVE_SKILL_ARAGORN_NUMENOREAN_WILL] =
        "Numenorean Will (Level @1) - +1 Armor and +3 damage per level for 5s/level. Wounds don't "
        "dampen the Flame while it lasts. @2 Spirit.",
    [ACTIVE_SKILL_ARAGORN_CALL_OF_THE_DEAD] = "Call of the Dead (Level @1) - Summon one Ghost "
                                              "Warrior/level. Each ghost hit stokes +2 Flame. @2 "
                                              "Spirit.",
    [ACTIVE_SKILL_HERBAL_HEALING] = "Herbal Healing (Level @1) - Heal with a Kingsfoil herb and "
                                    "stoke +15 Flame. Cannot be improved.",
};
static const char *const FrodoStockDescriptions[HERO_ACTIVE_SKILL_COUNT] = {
    [ACTIVE_SKILL_FRODO_KNIFE_TOSS] =
        "Knife Toss (Level @1) - Throw a knife for 8 +1/level. At 51+ Burden it's a shadow knife: "
        "+10% damage for 2s, +20% at 91+. @2 Spirit.",
    [ACTIVE_SKILL_FRODO_SNARE] = "Snare (Level @1) - Traps deal 5 +3/level damage, +1 trap/level. "
                                 "Snared foes don't feed your Burden. @2 Spirit.",
    [ACTIVE_SKILL_FRODO_GALADRIELS_CLOAK] =
        "Galadriel's Cloak (Level @1) - Foes ignore you for 2s +2s/level. Lifts 15 Burden, and "
        "Burden won't build while hidden. @2 Spirit.",
    [ACTIVE_SKILL_FRODO_THE_ONE_RING] =
        "The One Ring (Level @1) - Turn invisible for 5s +2s/level, but gain 50 Burden. Foes "
        "can't feed Burden while you're unseen. @2 Spirit.",
    [ACTIVE_SKILL_FRODO_RINGS_PERSUASION] =
        "Ring's Persuasion (Level @1) - 10%/level chance to turn nearby foes for 10s +5s/level. "
        "Adds 5 Burden. @2 Spirit.",
};

const char *skillExpansion_getStockActiveDescription(u32 heroId, u32 row)
{
    if (row >= HERO_ACTIVE_SKILL_COUNT)
    {
        return NULL;
    }
    switch (heroId)
    {
    case HERO_ID_ARAGORN:
        return AragornStockDescriptions[row];
    case HERO_ID_FRODO:
        return FrodoStockDescriptions[row];
    }
    return NULL;
}

#define NO_STAT_RECORDS                                                                            \
    .records = {{.statIndex = STAT_NONE}, {.statIndex = STAT_NONE}, {.statIndex = STAT_NONE}}

#define ELITE_PASSIVE(maxLevelValue, firstLevel, ...)                                              \
    .skill = {NO_STAT_RECORDS, .initialRequiredLevel = (firstLevel), .maxLevel = (maxLevelValue)}, \
    .requiredLevels = {firstLevel, __VA_ARGS__}

#define STAT_RECORD(stat, perLevelValue)                                                           \
    {                                                                                              \
        .statIndex = (stat),                                                                       \
        .perLevel = {perLevelValue, perLevelValue, perLevelValue, perLevelValue, perLevelValue}    \
    }
#define NO_STAT {.statIndex = STAT_NONE}

// A five-level passive that also grants up to two stats per level.
#define STAT_PASSIVE(firstLevel, firstRecord, secondRecord)                                        \
    .skill = {.records = {firstRecord, secondRecord, NO_STAT},                                     \
              .initialRequiredLevel = (firstLevel),                                                \
              .maxLevel = PASSIVE_SKILL_MAX_LEVEL},                                                \
    .requiredLevels = {(firstLevel),     (firstLevel) + 2, (firstLevel) + 4,                       \
                       (firstLevel) + 6, (firstLevel) + 8, -1}

const SkillExpansionElitePassive SkillExpansionElitePassives[SKILL_EXPANSION_ELITE_PASSIVE_COUNT] =
    {
        [SKILL_EXPANSION_PASSIVE_STRIDERS_RIPOSTE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 10, 12, 14, 16, 18, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_NIMBLE,
                .icon = AragornIconStridersRiposte,
                .description =
                    "Strider's Riposte (Level @1) - Ranger. When a foe's attack misses, 10%/level "
                    "chance to strike all foes nearby and stoke +8 Flame.",
            },
        [SKILL_EXPANSION_PASSIVE_BLADE_REFORGED - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 15, 17, 19, 21, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_DEATH_STRIKE,
                .icon = AragornIconBladeReforged,
                .description =
                    "The Blade Reforged (Level @1) - King. Below 30% health, the Flame fills at "
                    "once, or a burning Flame gains 4s. Every 90s -10s/level.",
            },
        [SKILL_EXPANSION_PASSIVE_HUNTERS_PATIENCE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 13, 15, 17, 19, 21, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_KEEN_EYES,
                .icon = AragornIconHuntersPatience,
                .description =
                    "Hunter's Patience (Level @1) - Ranger. Hold your blade 1.5s and Anduril "
                    "glints. Your next blow deals +20%/level and stokes +10 Flame.",
            },
        [SKILL_EXPANSION_PASSIVE_KINGS_PRESENCE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 12, 14, 16, 18, 20, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_IRON_WILL,
                .icon = AragornIconKingsPresence,
                .description =
                    "King's Presence (Level @1) - King. Your stuns last 0.5s longer/level. +2 "
                    "damage/level against stunned foes.",
            },
        [SKILL_EXPANSION_PASSIVE_HANDS_OF_THE_KING - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 21, 22, 23, 24, 25, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_HERB_LORE,
                .icon = AragornIconHandsOfTheKing,
                .description =
                    "Hands of the King (Level @1) - King. Kills during Flame of the West heal "
                    "you, and your co-op partner, 2%/level of max health.",
            },
        [SKILL_EXPANSION_PASSIVE_RESTLESS_DEAD - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 11, 13, 15, 17, 19, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_WRAITHSLAYER,
                .icon = AragornIconRestlessDead,
                .description =
                    "Restless Dead (Level @1) - Dead. Ghosts burst as they fade, hitting nearby "
                    "foes for 6 +6/level, stunning them and stoking +5 Flame.",
            },
        [SKILL_EXPANSION_PASSIVE_TRACK_THE_QUARRY - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 17, 19, 21, 23, 25, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_HARDY,
                .icon = AragornIconTrackTheQuarry,
                .description =
                    "Track the Quarry (Level @1) - Ranger. When your marked foe dies, the Mark "
                    "leaps to the nearest foe with 2s/level more.",
            },
        [SKILL_EXPANSION_PASSIVE_THE_DEAD_RIDE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 14, 16, 18, 20, 22, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_BLADEMASTER,
                .icon = AragornIconTheDeadRide,
                .description =
                    "The Dead Ride (Level @1) - Dead. Kills within 3s, yours or your ghosts', "
                    "chain. Every 7 -1/level chained kills raises a ghost.",
            },
        [SKILL_EXPANSION_PASSIVE_LORD_OF_THE_DEAD - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 20, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_RAGE_OF_THE_NORTH,
                .icon = AragornIconLordOfTheDead,
                .description =
                    "Lord of the Dead (Level @1) - Dead. Foes you slay have a 10% chance to raise "
                    "a ghost that fights for you.",
            },
        [SKILL_EXPANSION_PASSIVE_SERVANT_OF_THE_RING - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 2, 4, 6, 8, 10, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_THE_PRECIOUS,
                .description = "Servant of the Ring (Level @1) - Each kill feeds you +3/level more "
                               "Spirit, and each foe you terrify +1/level more.",
            },
        [SKILL_EXPANSION_PASSIVE_WRAITH_FORM - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 3, 5, 7, 9, 11, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_WRAITHSLAYER,
                RINGWRAITH_ICON(WraithForm).description =
                    "Wraith Form (Level @1) - Blows pass through you: -6%/level melee "
                    "damage taken. Arrows, fire and magic deal +20%.",
            },
        [SKILL_EXPANSION_PASSIVE_LORD_OF_TERROR - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 4, 6, 8, 10, 12, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_FEARLESS,
                .description =
                    "Lord of Terror (Level @1) - Hits add +5/level Dread. Frightened foes "
                    "deal 4%/level less damage.",
            },
        [SKILL_EXPANSION_PASSIVE_SHADOW_HUNGER - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 2, 5, 8, 11, 14, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_DIRTY_CLAWS,
                .description = "Shadow Hunger (Level @1) - Your drains heal +20%/level more.",
            },
        [SKILL_EXPANSION_PASSIVE_MORGUL_SORCERY - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 6, 8, 10, 12, 14, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_SERVANT_OF_THE_SECRET_FIRE,
                .description =
                    "Morgul Sorcery (Level @1) - Thralls linger +3s/level. One more thrall "
                    "may serve you at levels 2 and 4.",
            },
        [SKILL_EXPANSION_PASSIVE_FEARS_REACH - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 3, 6, 9, 12, 15, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_KEEN_EYES,
                .description =
                    "Fear's Reach (Level @1) - Your aura of dread spreads +10/level wider.",
            },
        [SKILL_EXPANSION_PASSIVE_WITCH_KINGS_MACE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 5, 7, 9, 11, 13, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_DEATH_STRIKE,
                .description = "Witch-king's Mace (Level @1) - Melee deals +6%/level to frightened "
                               "foes, doubled against the terrified.",
            },
        [SKILL_EXPANSION_PASSIVE_UNDYING_MALICE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 13, 15, 17, 19, 21, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_LAST_STAND,
                .description = "Undying Malice (Level @1) - No Living Man lasts +1s/level and its "
                               "kills heal +3%/level more.",
            },
        [SKILL_EXPANSION_PASSIVE_RINGBOUND - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 15, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_SPIRIT_OF_MIDDLE_EARTH,
                .description =
                    "Ringbound (Level @1) - Begin missions at full Spirit. The Unseen World "
                    "refunds half its cost.",
            },
        [SKILL_EXPANSION_PASSIVE_FADING - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 15, 17, 19, 21, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_NIMBLE,
                .description = "Fading (Level @1) - The Unseen World lasts +0.5s/level and refunds "
                               "10%/level of its cost.",
            },
        [SKILL_EXPANSION_PASSIVE_HOST_OF_ANGMAR - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 16, 18, 20, 22, 24, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_RAGE_OF_THE_NORTH,
                .description = "Host of Angmar (Level @1) - One more thrall/level may serve you.",
            },
        [SKILL_EXPANSION_PASSIVE_DREAD_LORD - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 17, 19, 21, 23, 25, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_FEARLESS,
                RINGWRAITH_ICON(DreadLord).description =
                    "Dread Lord (Level @1) - Frightened foes take +4%/level damage from you "
                    "and your thralls, doubled against the terrified.",
            },
        [SKILL_EXPANSION_PASSIVE_FEAST_OF_FEAR - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 18, 20, 22, 24, 26, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_DIRTY_CLAWS,
                .description =
                    "Feast of Fear (Level @1) - Slaying a terrified foe heals 2%/level of "
                    "your health and grants +2 Spirit/level.",
            },
        [SKILL_EXPANSION_PASSIVE_GRAVE_PACT - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 19, 21, 23, 25, 27, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_LAST_STAND,
                .description = "Grave Pact (Level @1) - Thralls linger +1s/level. A fading thrall "
                               "bursts for 6 +3/level damage and Dread.",
            },
        [SKILL_EXPANSION_PASSIVE_KNIFE_IN_THE_DARK - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 20, 22, 24, 26, 28, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_DEATH_STRIKE,
                .description = "Knife in the Dark (Level @1) - An Unseen kill hides you again for "
                               "0.75s +0.25s/level.",
            },
        [SKILL_EXPANSION_PASSIVE_MASTER_OF_THE_DEAD - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 30, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_SERVANT_OF_THE_SECRET_FIRE,
                RINGWRAITH_ICON(MasterOfTheDead).description =
                    "Master of the Dead (Level @1) - Your thralls never fade, but your own "
                    "blows deal 30% less.",
            },
        [SKILL_EXPANSION_PASSIVE_DEATH_FROM_SHADOW - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 31, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_BLADEMASTER,
                .description = "Death from Shadow (Level @1) - Unseen blows deal x5 instead of x3, "
                               "but seen blows deal 40% less.",
            },
        [SKILL_EXPANSION_PASSIVE_ENDLESS_NIGHT - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 33, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_IRON_WILL,
                RINGWRAITH_ICON(EndlessNight).description =
                    "Endless Night (Level @1) - Dread never fades and your aura spreads "
                    "twice as far, but you can never be Unseen.",
            },
        [SKILL_EXPANSION_PASSIVE_WEIGHT_OF_THE_RING - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 11, 13, 15, 17, 19, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_THE_PRECIOUS,
                .icon = FrodoIconWeightOfTheRing,
                .description =
                    "Weight of the Ring (Level @1) - Evil. Deal +2%/level damage for every "
                    "10 Burden.",
            },
        [SKILL_EXPANSION_PASSIVE_STRENGTH_OF_WILL - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 10, 12, 14, 16, 18, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_IRON_WILL,
                .icon = FrodoIconStrengthOfWill,
                .description = "Strength of Will (Level @1) - Good. Gain 10%/level less Burden.",
            },
        [SKILL_EXPANSION_PASSIVE_RIDDLES_IN_THE_DARK - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 12, 14, 16, 18, 20, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_WISE,
                .icon = FrodoIconRiddlesInTheDark,
                .description = "Riddles in the Dark (Level @1) - Tricks. Foes you turn stay turned "
                               "1s/level longer.",
            },
        [SKILL_EXPANSION_PASSIVE_RINGS_HUNGER - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 15, 17, 19, 21, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_DEATH_STRIKE,
                .icon = FrodoIconRingsHunger,
                .description = "The Ring's Hunger (Level @1) - Evil. At 51+ Burden or wearing the "
                               "Ring, hits heal you 3%/level of the damage dealt.",
            },
        [SKILL_EXPANSION_PASSIVE_STING_GLOWS_BLUE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 14, 16, 18, 20, 22, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_HARDY,
                .icon = FrodoIconStingGlowsBlue,
                .description =
                    "Sting Glows Blue (Level @1) - Good. Below 51 Burden, Sting glows near "
                    "foes. Glowing melee hits deal +2/level and restore 1/level Spirit.",
            },
        [SKILL_EXPANSION_PASSIVE_TRICKSY_HOBBIT - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 16, 18, 20, 22, 24, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_LUCK,
                .icon = FrodoIconTricksyHobbit,
                .description =
                    "Tricksy Hobbit (Level @1) - Tricks. Your hits have a 4%/level chance to "
                    "turn the foe for 3s.",
            },
        [SKILL_EXPANSION_PASSIVE_CLAIM_IT - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_LAST_STAND,
                .icon = FrodoIconClaimIt,
                .description =
                    "Claim It (Level @1) - Evil. At 100 Burden, or once per mission on a "
                    "kill, become a shadow 10s: free skills, +50% damage. Burden then "
                    "drops to 60.",
            },
        [SKILL_EXPANSION_PASSIVE_UNWAVERING - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 22, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_FEARLESS,
                .icon = FrodoIconUnwavering,
                .description =
                    "Unwavering (Level @1) - Good. After 10s with no Burden you shine. Your "
                    "next melee hit deals x3 and stuns for 3s, and Phial is free.",
            },
        [SKILL_EXPANSION_PASSIVE_PRECIOUS_LIES - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 24, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_WISDOM_OF_THE_AGES,
                .icon = FrodoIconPreciousLies,
                .description =
                    "Precious Lies (Level @1) - Tricks. Sow Discord turns every foe in range "
                    "for 50% longer.",
            },
        [SKILL_EXPANSION_PASSIVE_POOR_SMEAGOL - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 10, 12, 14, 16, 18, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_WHY_DOES_IT_HURT_SMEAGOL,
                .icon = SmeagolIconPoorSmeagol,
                .description = "Poor Smeagol (Level @1) - Smeagol. Gollum's Grip builds 15%/level "
                               "slower in battle.",
            },
        [SKILL_EXPANSION_PASSIVE_HUNGRY - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 11, 13, 15, 17, 19, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_DIRTY_CLAWS,
                .icon = SmeagolIconHungry,
                .description = "Hungry (Level @1) - Gollum. Deal +1%/level damage for every 10 "
                               "grip.",
            },
        [SKILL_EXPANSION_PASSIVE_SNIFF_IT_OUT - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 12, 14, 16, 18, 20, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_LUCK,
                .icon = SmeagolIconSniffItOut,
                .description = "Sniff It Out (Level @1) - Precious. Sparkles mark the nearest "
                               "unopened chest or dropped loot within 80 +16/level.",
            },
        [SKILL_EXPANSION_PASSIVE_NICE_MASTER - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 14, 16, 18, 20, 22, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_IRON_WILL,
                .icon = SmeagolIconNiceMaster,
                .description = "Nice Master (Level @1) - Smeagol. Play Dead heals 4%/level of your "
                               "max HP.",
            },
        [SKILL_EXPANSION_PASSIVE_RAT_CATCHER - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 15, 17, 19, 21, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_BERSERKER,
                .icon = SmeagolIconRatCatcher,
                .description = "Rat Catcher (Level @1) - Gollum. Throttle kills add 10 grip and "
                               "restore 4/level Spirit.",
            },
        [SKILL_EXPANSION_PASSIVE_FINDERS_KEEPERS - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 16, 18, 20, 22, 24, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_THE_PRECIOUS,
                .icon = SmeagolIconFindersKeepers,
                .description = "Finders Keepers (Level @1) - Precious. Each foe near the Ring when "
                               "you grab it has a 15%/level chance to roll a loot drop.",
            },
        [SKILL_EXPANSION_PASSIVE_SMEAGOL_IS_FREE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 22, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_NIMBLE,
                .icon = SmeagolIconSmeagolIsFree,
                .description = "Smeagol Is Free (Level @1) - Smeagol. Mash A to cut a takeover in "
                               "half: you're unharmed 5s and your Spirit fills.",
            },
        [SKILL_EXPANSION_PASSIVE_WE_WANTS_IT - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_ORCSLAYER,
                .icon = SmeagolIconWeWantsIt,
                .description = "We Wants It (Level @1) - Gollum. Takeovers last 2s longer, you "
                               "can steer them, and each kill adds 1s.",
            },
        [SKILL_EXPANSION_PASSIVE_MY_PRECIOUS - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 24, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_FLEET_OF_FOOT,
                .icon = SmeagolIconMyPrecious,
                .description = "My Precious! (Level @1) - Precious. At 70+ grip or during a "
                               "takeover, kills roll an extra loot drop.",
            },
        [SKILL_EXPANSION_PASSIVE_GREEN_THUMB - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 10, 12, 14, 16, 18, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_HERB_LORE,
                .icon = SamIconGreenThumb,
                .description = "Green Thumb (Level @1) - Gardener. Your plants grow 20%/level "
                               "faster.",
            },
        [SKILL_EXPANSION_PASSIVE_STUBBORN - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 11, 13, 15, 17, 19, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_IRON_WILL,
                .icon = SamIconStubborn,
                .description = "Stubborn (Level @1) - Stout. Take 5%/level less damage near your "
                               "plants or stew pot.",
            },
        [SKILL_EXPANSION_PASSIVE_LEMBAS_CRUMBS - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 12, 14, 16, 18, 20, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_GALADRIELS_BLESSING,
                .icon = SamIconLembasCrumbs,
                .description = "Lembas Crumbs (Level @1) - Cook. Harvesting a tater heals "
                               "3%/level of your max HP.",
            },
        [SKILL_EXPANSION_PASSIVE_GAFFERS_LORE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 14, 16, 18, 20, 22, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_WISE,
                .icon = SamIconGaffersLore,
                .description = "Gaffer's Lore (Level @1) - Gardener. Ripe plants last 2s/level "
                               "longer before they wilt.",
            },
        [SKILL_EXPANSION_PASSIVE_SHIRE_GRIT - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 15, 17, 19, 21, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_FIGHTERS_RESOLVE,
                .icon = SamIconShireGrit,
                .description = "Shire Grit (Level @1) - Stout. Each parried hit adds 3/level "
                               "damage to your next melee hit.",
            },
        [SKILL_EXPANSION_PASSIVE_FINE_SEASONING - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 16, 18, 20, 22, 24, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_SPIRIT_OF_MIDDLE_EARTH,
                .icon = SamIconFineSeasoning,
                .description = "Fine Seasoning (Level @1) - Cook. Each tater you harvest or "
                               "throw restores 3/level Spirit.",
            },
        [SKILL_EXPANSION_PASSIVE_MASTER_GARDENER - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 22, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_WOODSMAN,
                .icon = SamIconMasterGardener,
                .description = "Master Gardener (Level @1) - Gardener. Tend a fourth plant at "
                               "once.",
            },
        [SKILL_EXPANSION_PASSIVE_I_CAN_CARRY_YOU - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_LAST_STAND,
                .icon = SamIconICanCarryYou,
                .description = "I Can Carry You (Level @1) - Stout. Once per area, a hit that "
                               "drops you below 25% HP heals 40% and leaves you unharmed 2s.",
            },
        [SKILL_EXPANSION_PASSIVE_TATERS_FOR_ALL - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 24, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_BERSERKER,
                .icon = SamIconTatersForAll,
                .description = "Taters for All (Level @1) - Cook. Deal +20% damage while near "
                               "your stew pot and for 1.5s after.",
            },
        [SKILL_EXPANSION_PASSIVE_LORE_OF_COLOURS - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 10, 12, 14, 16, 18, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_WISE,
                .description = "Lore of Colours (Level @1) - Curunir. Refractions are 15%/level "
                               "stronger.",
            },
        [SKILL_EXPANSION_PASSIVE_FIRES_OF_INDUSTRY - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 11, 13, 15, 17, 19, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_BERSERKER,
                .description = "Fires of Industry (Level @1) - White Hand. Your burns deal "
                               "+1/level damage each second and last 2s/level longer.",
            },
        [SKILL_EXPANSION_PASSIVE_CUTPURSE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 12, 14, 16, 18, 20, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_ACCURACY,
                .description = "Cutpurse (Level @1) - Sharkey. When a cursed foe dies, steal "
                               "5/level Spirit and the curse jumps to a foe nearby.",
            },
        [SKILL_EXPANSION_PASSIVE_RING_LORE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 14, 16, 18, 20, 22, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_SPIRIT_OF_MIDDLE_EARTH,
                .description = "Ring-lore (Level @1) - Curunir. Each refraction restores "
                               "4/level Spirit.",
            },
        [SKILL_EXPANSION_PASSIVE_MACHINE_OF_WAR - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 15, 17, 19, 21, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_FIGHTERS_RESOLVE,
                .description = "Machine of War (Level @1) - White Hand. Each foe your spells "
                               "kill restores 2/level Spirit.",
            },
        [SKILL_EXPANSION_PASSIVE_SPITE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 16, 18, 20, 22, 24, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_LAST_STAND,
                .description = "Spite (Level @1) - Sharkey. Below 40% HP your spells deal "
                               "+6%/level damage.",
            },
        [SKILL_EXPANSION_PASSIVE_HEAD_OF_THE_COUNCIL - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 22, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_SERVANT_OF_THE_SECRET_FIRE,
                .description = "Head of the White Council (Level @1) - Curunir. The spell after "
                               "a refraction costs no Spirit instead of half.",
            },
        [SKILL_EXPANSION_PASSIVE_POWDER_OF_ORTHANC - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_RANGEMASTER,
                .description = "Powder of Orthanc (Level @1) - White Hand. Foes your spells "
                               "kill explode, hurting foes near them.",
            },
        [SKILL_EXPANSION_PASSIVE_PARTING_SHOT - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 24, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_IRON_WILL,
                .description = "Parting Shot (Level @1) - Sharkey. Falling below 30% HP lets "
                               "loose a free Spite blast and curses a foe. Once per 20s.",
            },
        [SKILL_EXPANSION_PASSIVE_DWARVEN_MAIL - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 1, 3, 5, 7, 9, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_HARDY,
                .icon = GimliIconDwarvenMail,
                .description = "Dwarven Mail (Kills @1) - Stone. Take 2%/level less damage for "
                               "each foe right next to you, up to 4.",
            },
        [SKILL_EXPANSION_PASSIVE_GRUDGEBEARER - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 2, 4, 6, 8, 10, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_ORCSLAYER,
                .icon = GimliIconGrudgebearer,
                .description = "Grudgebearer (Kills @1) - Fury. Foes marked by Book of Grudges "
                               "take a further +4%/level damage.",
            },
        [SKILL_EXPANSION_PASSIVE_FRIENDLY_WAGER - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 3, 5, 7, 9, 11, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_LUCK,
                .icon = GimliIconFriendlyWager,
                .description = "Friendly Wager (Kills @1) - Tally. Kill 3 foes within 5s to "
                               "deal +5%/level damage for 8s.",
            },
        [SKILL_EXPANSION_PASSIVE_NOT_WITHOUT_ME - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 6, 9, 12, 15, 18, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_FIGHTERS_RESOLVE,
                .icon = GimliIconNotWithoutMe,
                .description = "Not Without Me (Kills @1) - Stone. In co-op, take 8%/level of the "
                               "hits your nearby partner takes.",
            },
        [SKILL_EXPANSION_PASSIVE_BLOOD_ON_THE_AXE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 7, 10, 13, 16, 19, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_BERSERKER,
                .icon = GimliIconBloodOnTheAxe,
                .description = "Blood on the Axe (Kills @1) - Fury. Kills during Dwarven Rage "
                               "add 0.5s/level to it.",
            },
        [SKILL_EXPANSION_PASSIVE_ORC_COUNT - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 8, 11, 14, 17, 20, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_DWARF_SENSE,
                .icon = GimliIconOrcCount,
                .description = "Orc-Count (Kills @1) - Tally. Each Tally mark (every 10 kills) "
                               "restores 4/level Spirit.",
            },
        [SKILL_EXPANSION_PASSIVE_STUBBORN_AS_STONE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 19, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_IRON_WILL,
                .icon = GimliIconStubbornAsStone,
                .description = "Stubborn as Stone (Kills @1) - Stone. Hits never make you "
                               "flinch.",
            },
        [SKILL_EXPANSION_PASSIVE_DURINS_FOLK - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 20, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_LAST_STAND,
                .icon = GimliIconDurinsFolk,
                .description = "Durin's Folk Don't Fall (Kills @1) - Fury. Once per Dwarven Rage, "
                               "a killing blow leaves you at 1 HP.",
            },
        [SKILL_EXPANSION_PASSIVE_FORTY_TWO - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 21, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_AXEMASTER,
                .icon = GimliIconFortyTwo,
                .description = "Forty-Two! (Kills @1) - Tally. Every 42nd kill (counted down "
                               "aloud) sets off a free quake and fills your Spirit.",
            },
        [SKILL_EXPANSION_PASSIVE_WANDERING_WIZARD - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 10, 12, 14, 16, 18, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_WISE,
                .icon = GandalfIconWanderingWizard,
                .description =
                    "Wandering Wizard (Level @1) - Grey. The Grey gains 1 Spirit every 3s "
                    "-0.4s/level.",
            },
        [SKILL_EXPANSION_PASSIVE_ROBES_OF_WHITE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 11, 13, 15, 17, 19, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_HARDY,
                .icon = GandalfIconRobesOfWhite,
                .description =
                    "Robes of White (Level @1) - White. The White takes 5%/level less damage.",
            },
        [SKILL_EXPANSION_PASSIVE_KINDLER_OF_HEARTS - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 12, 14, 16, 18, 20, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_FIGHTERS_RESOLVE,
                .icon = GandalfIconKindlerOfHearts,
                .description = "Kindler of Hearts (Level @1) - Narya. Swap stuns and smoke last "
                               "0.2s/level longer and the Return reaches 12/level farther.",
            },
        [SKILL_EXPANSION_PASSIVE_OLD_TRICKS - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 14, 16, 18, 20, 22, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_LUCK,
                .icon = GandalfIconOldTricks,
                .description =
                    "Old Tricks (Level @1) - Grey. Grey halves of your page 2 skills cost "
                    "10%/level less Spirit.",
            },
        [SKILL_EXPANSION_PASSIVE_UNBENDING - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 15, 17, 19, 21, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_IRON_WILL,
                .icon = GandalfIconUnbending,
                .description =
                    "Unbending (Level @1) - White. The White's Spirit drain is 13%/level "
                    "slower.",
            },
        [SKILL_EXPANSION_PASSIVE_QUICKENED - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 16, 18, 20, 22, 24, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_FLEET_OF_FOOT,
                .icon = GandalfIconQuickened,
                .description =
                    "Quickened (Level @1) - Narya. Swaps recover 0.5s/level faster and the "
                    "next skill after one costs 10%/level less.",
            },
        [SKILL_EXPANSION_PASSIVE_THE_FALL - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 20, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_LAST_STAND,
                .icon = GandalfIconTheFall,
                .description =
                    "The Fall (Level @1) - Grey. Once per area a killing blow on the Grey "
                    "hides you 3s, then you return as the White with full Spirit.",
            },
        [SKILL_EXPANSION_PASSIVE_WHITE_RIDER - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 21, 22, 23, 24, 25, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_SPIRIT_OF_MIDDLE_EARTH,
                .icon = GandalfIconWhiteRider,
                .description =
                    "The White Rider (Level @1) - White. Each kill as the White restores "
                    "3/level Spirit.",
            },
        [SKILL_EXPANSION_PASSIVE_SECRET_FIRE - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_SERVANT_OF_THE_SECRET_FIRE,
                .icon = GandalfIconSecretFire,
                .description =
                    "Servant of the Secret Fire (Level @1) - Narya. Every swap looses a fire "
                    "nova for 10 + your level.",
            },
        [SKILL_EXPANSION_PASSIVE_ROBES_OF_MANY_COLOURS - PASSIVE_SKILL_COUNT] =
            {
                STAT_PASSIVE(1, STAT_RECORD(STAT_MELEE_ARMOR, 2),
                             STAT_RECORD(STAT_MISSILE_ARMOR, 2)),
                .iconPassiveSkillId = PASSIVE_SKILL_BATTLE_SCARRED,
                .description = "Robes of Many Colours (Level @1) - White. Take 2% less damage per "
                               "level for each color in your Prism. +2 armor per level.",
            },
        [SKILL_EXPANSION_PASSIVE_KEEPER_OF_ORTHANC - PASSIVE_SKILL_COUNT] =
            {
                STAT_PASSIVE(4, STAT_RECORD(STAT_MAX_HP, 15), NO_STAT),
                .iconPassiveSkillId = PASSIVE_SKILL_HARDY,
                .description = "Keeper of Orthanc (Level @1) - White. Remedy heals 10% more per "
                               "level. Overhealing becomes Spirit. +15 max HP per level.",
            },
        [SKILL_EXPANSION_PASSIVE_WISDOM_OF_THE_ISTARI - PASSIVE_SKILL_COUNT] =
            {
                STAT_PASSIVE(7, STAT_RECORD(STAT_SPIRIT_REGEN_PERCENT, 20), NO_STAT),
                .iconPassiveSkillId = PASSIVE_SKILL_SPIRIT_OF_MIDDLE_EARTH,
                .description = "Wisdom of the Istari (Level @1) - White. The Prism holds its "
                               "colors 1s longer per level. +20% Spirit regen per level.",
            },
        [SKILL_EXPANSION_PASSIVE_KINDLING - PASSIVE_SKILL_COUNT] =
            {
                STAT_PASSIVE(2, NO_STAT, NO_STAT),
                .iconPassiveSkillId = PASSIVE_SKILL_SERVANT_OF_THE_SECRET_FIRE,
                .description = "Kindling (Level @1) - Isengard. Your spell hits deal 6% more per "
                               "level to burning foes.",
            },
        [SKILL_EXPANSION_PASSIVE_WILDFIRE - PASSIVE_SKILL_COUNT] =
            {
                STAT_PASSIVE(5, NO_STAT, NO_STAT),
                .iconPassiveSkillId = PASSIVE_SKILL_WISDOM_OF_THE_AGES,
                .description = "Wildfire (Level @1) - Isengard. When a burning foe dies, its fire "
                               "leaps to a foe nearby and burns for 3s +1s per level.",
            },
        [SKILL_EXPANSION_PASSIVE_STAFF_OF_ORTHANC - PASSIVE_SKILL_COUNT] =
            {
                STAT_PASSIVE(8, STAT_RECORD(STAT_ACCURACY, 1), NO_STAT),
                .iconPassiveSkillId = PASSIVE_SKILL_ACCURACY,
                .description = "Staff of Orthanc (Level @1) - Isengard. Staff hits burn a foe for "
                               "1 +1/level each second for 3s. +1 accuracy per level.",
            },
        [SKILL_EXPANSION_PASSIVE_HONEYED_WORDS - PASSIVE_SKILL_COUNT] =
            {
                STAT_PASSIVE(3, STAT_RECORD(STAT_FEAR_RESIST_PERCENT, 6), NO_STAT),
                .iconPassiveSkillId = PASSIVE_SKILL_FEARLESS,
                .description = "Honeyed Words (Level @1) - Voice. Your stuns and holds last 10% "
                               "longer per level. +6% fear resist per level.",
            },
        [SKILL_EXPANSION_PASSIVE_VOICE_OF_COMMAND - PASSIVE_SKILL_COUNT] =
            {
                STAT_PASSIVE(6, NO_STAT, NO_STAT),
                .iconPassiveSkillId = PASSIVE_SKILL_IRON_WILL,
                .description = "Voice of Command (Level @1) - Voice. Stunned foes take 6% more per "
                               "level from your spell hits.",
            },
        [SKILL_EXPANSION_PASSIVE_TREACHERY - PASSIVE_SKILL_COUNT] =
            {
                STAT_PASSIVE(9, NO_STAT, NO_STAT),
                .iconPassiveSkillId = PASSIVE_SKILL_WISE,
                .description = "Treachery (Level @1) - Voice. Staff hits on stunned foes restore 1 "
                               "Spirit per level.",
            },
        [SKILL_EXPANSION_PASSIVE_ELF_SIGHT - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 10, 12, 14, 16, 18, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_KEEN_EYES,
                .icon = LegolasIconElfSight,
                .description = "Elf-Sight (Level @1) - Eye. The glint stays open 1 frame longer per "
                               "level.",
            },
        [SKILL_EXPANSION_PASSIVE_SWIFT_NOCK - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 11, 13, 15, 17, 19, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_NIMBLE,
                .icon = LegolasIconSwiftNock,
                .description = "Swift Nock (Level @1) - Quiver. Nocking an arrow costs 8%/level less "
                               "Spirit.",
            },
        [SKILL_EXPANSION_PASSIVE_ELVEN_CLOAK - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 12, 14, 16, 18, 20, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_GALADRIELS_BLESSING,
                .icon = LegolasIconElvenCloak,
                .description = "Elven Cloak (Level @1) - Grace. For 2s after a True Shot you take 6%/level "
                               "less damage.",
            },
        [SKILL_EXPANSION_PASSIVE_STEADY_HAND - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 14, 16, 18, 20, 22, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_ACCURACY,
                .icon = LegolasIconSteadyHand,
                .description = "Steady Hand (Level @1) - Eye. True Shot arrows deal +5%/level more per "
                               "streak step.",
            },
        [SKILL_EXPANSION_PASSIVE_FLETCHER - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 15, 17, 19, 21, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_RANGEMASTER,
                .icon = LegolasIconFletcher,
                .description = "Fletcher (Level @1) - Quiver. A True loose of a nocked arrow refunds "
                               "10%/level of its cost.",
            },
        [SKILL_EXPANSION_PASSIVE_POINT_BLANK - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(PASSIVE_SKILL_MAX_LEVEL, 16, 18, 20, 22, 24, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_ARROW_PARRY,
                .icon = LegolasIconPointBlank,
                .description = "Point Blank (Level @1) - Grace. True Shots knock near foes back and stun "
                               "them 0.5s +0.1s/level.",
            },
        [SKILL_EXPANSION_PASSIVE_UNERRING - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 20, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_ARCHER_OF_MIRKWOOD,
                .icon = LegolasIconUnerring,
                .description = "Unerring (Level @1) - Eye. At a streak of 5 every True Shot also looses a "
                               "free half Piercing Shot.",
            },
        [SKILL_EXPANSION_PASSIVE_TWIN_NOCK - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 21, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_DEATH_STRIKE,
                .icon = LegolasIconTwinNock,
                .description = "Twin Nock (Level @1) - Quiver. A True loose fires the nocked arrow twice, "
                               "the second at half.",
            },
        [SKILL_EXPANSION_PASSIVE_UNSHAKEN - PASSIVE_SKILL_COUNT] =
            {
                ELITE_PASSIVE(1, 23, -1),
                .iconPassiveSkillId = PASSIVE_SKILL_FEARLESS,
                .icon = LegolasIconUnshaken,
                .description = "Unshaken (Level @1) - Grace. Hits no longer break your draw.",
            },
};

const u8 SkillExpansionPassiveIds[SKILL_EXPANSION_HERO_COUNT][SKILL_EXPANSION_NEW_PASSIVE_COUNT] = {
    [HERO_ID_FRODO] =
        {
            SKILL_EXPANSION_PASSIVE_STRENGTH_OF_WILL,
            SKILL_EXPANSION_PASSIVE_WEIGHT_OF_THE_RING,
            SKILL_EXPANSION_PASSIVE_RIDDLES_IN_THE_DARK,
            SKILL_EXPANSION_PASSIVE_STING_GLOWS_BLUE,
            SKILL_EXPANSION_PASSIVE_RINGS_HUNGER,
            SKILL_EXPANSION_PASSIVE_TRICKSY_HOBBIT,
            SKILL_EXPANSION_PASSIVE_UNWAVERING,
            SKILL_EXPANSION_PASSIVE_CLAIM_IT,
            SKILL_EXPANSION_PASSIVE_PRECIOUS_LIES,
        },
    [HERO_ID_LEGOLAS] =
        {
            SKILL_EXPANSION_PASSIVE_ELF_SIGHT,
            SKILL_EXPANSION_PASSIVE_SWIFT_NOCK,
            SKILL_EXPANSION_PASSIVE_ELVEN_CLOAK,
            SKILL_EXPANSION_PASSIVE_STEADY_HAND,
            SKILL_EXPANSION_PASSIVE_FLETCHER,
            SKILL_EXPANSION_PASSIVE_POINT_BLANK,
            SKILL_EXPANSION_PASSIVE_UNERRING,
            SKILL_EXPANSION_PASSIVE_TWIN_NOCK,
            SKILL_EXPANSION_PASSIVE_UNSHAKEN,
        },
    [HERO_ID_ARAGORN] =
        {
            SKILL_EXPANSION_PASSIVE_STRIDERS_RIPOSTE,
            SKILL_EXPANSION_PASSIVE_RESTLESS_DEAD,
            SKILL_EXPANSION_PASSIVE_KINGS_PRESENCE,
            SKILL_EXPANSION_PASSIVE_HUNTERS_PATIENCE,
            SKILL_EXPANSION_PASSIVE_THE_DEAD_RIDE,
            SKILL_EXPANSION_PASSIVE_BLADE_REFORGED,
            SKILL_EXPANSION_PASSIVE_TRACK_THE_QUARRY,
            SKILL_EXPANSION_PASSIVE_LORD_OF_THE_DEAD,
            SKILL_EXPANSION_PASSIVE_HANDS_OF_THE_KING,
        },
    [HERO_ID_GANDALF] =
        {
            SKILL_EXPANSION_PASSIVE_WANDERING_WIZARD,
            SKILL_EXPANSION_PASSIVE_ROBES_OF_WHITE,
            SKILL_EXPANSION_PASSIVE_KINDLER_OF_HEARTS,
            SKILL_EXPANSION_PASSIVE_OLD_TRICKS,
            SKILL_EXPANSION_PASSIVE_UNBENDING,
            SKILL_EXPANSION_PASSIVE_QUICKENED,
            SKILL_EXPANSION_PASSIVE_THE_FALL,
            SKILL_EXPANSION_PASSIVE_WHITE_RIDER,
            SKILL_EXPANSION_PASSIVE_SECRET_FIRE,
        },
    [HERO_ID_EOWYN] =
        {
            PASSIVE_SKILL_FEARLESS,
            PASSIVE_SKILL_ACCURACY,
            PASSIVE_SKILL_DEATH_STRIKE,
            PASSIVE_SKILL_BLADEMASTER,
            PASSIVE_SKILL_LUCK,
            PASSIVE_SKILL_ORCSLAYER,
            PASSIVE_SKILL_HARDY,
            PASSIVE_SKILL_LAST_STAND,
            PASSIVE_SKILL_GALADRIELS_BLESSING,
        },
    [HERO_ID_GIMLI] =
        {
            SKILL_EXPANSION_PASSIVE_DWARVEN_MAIL,
            SKILL_EXPANSION_PASSIVE_GRUDGEBEARER,
            SKILL_EXPANSION_PASSIVE_FRIENDLY_WAGER,
            SKILL_EXPANSION_PASSIVE_NOT_WITHOUT_ME,
            SKILL_EXPANSION_PASSIVE_BLOOD_ON_THE_AXE,
            SKILL_EXPANSION_PASSIVE_ORC_COUNT,
            SKILL_EXPANSION_PASSIVE_STUBBORN_AS_STONE,
            SKILL_EXPANSION_PASSIVE_DURINS_FOLK,
            SKILL_EXPANSION_PASSIVE_FORTY_TWO,
        },
    [HERO_ID_SAM] =
        {
            SKILL_EXPANSION_PASSIVE_GREEN_THUMB,
            SKILL_EXPANSION_PASSIVE_STUBBORN,
            SKILL_EXPANSION_PASSIVE_LEMBAS_CRUMBS,
            SKILL_EXPANSION_PASSIVE_GAFFERS_LORE,
            SKILL_EXPANSION_PASSIVE_SHIRE_GRIT,
            SKILL_EXPANSION_PASSIVE_FINE_SEASONING,
            SKILL_EXPANSION_PASSIVE_MASTER_GARDENER,
            SKILL_EXPANSION_PASSIVE_I_CAN_CARRY_YOU,
            SKILL_EXPANSION_PASSIVE_TATERS_FOR_ALL,
        },
    [HERO_ID_SMEAGOL] =
        {
            SKILL_EXPANSION_PASSIVE_POOR_SMEAGOL,
            SKILL_EXPANSION_PASSIVE_HUNGRY,
            SKILL_EXPANSION_PASSIVE_SNIFF_IT_OUT,
            SKILL_EXPANSION_PASSIVE_NICE_MASTER,
            SKILL_EXPANSION_PASSIVE_RAT_CATCHER,
            SKILL_EXPANSION_PASSIVE_FINDERS_KEEPERS,
            SKILL_EXPANSION_PASSIVE_SMEAGOL_IS_FREE,
            SKILL_EXPANSION_PASSIVE_WE_WANTS_IT,
            SKILL_EXPANSION_PASSIVE_MY_PRECIOUS,
        },

};

// Page 2 rows 7..12 laid out Good | Evil | Tricks on the tree's top row, then Good | Tricks, then
// Evil in the bottom slot.
static const u8 FrodoActiveBuilds[SKILL_EXPANSION_NEW_ACTIVE_COUNT] = {
    SKILL_EXPANSION_BUILD_GOOD, SKILL_EXPANSION_BUILD_EVIL,   SKILL_EXPANSION_BUILD_TRICKS,
    SKILL_EXPANSION_BUILD_GOOD, SKILL_EXPANSION_BUILD_TRICKS, SKILL_EXPANSION_BUILD_EVIL,
};

// Smeagol's page 2 rows laid out Smeagol | Gollum | Precious, then Smeagol | Gollum, then Precious.
static const u8 SmeagolActiveBuilds[SKILL_EXPANSION_NEW_ACTIVE_COUNT] = {
    SKILL_EXPANSION_BUILD_GOOD, SKILL_EXPANSION_BUILD_EVIL, SKILL_EXPANSION_BUILD_TRICKS,
    SKILL_EXPANSION_BUILD_GOOD, SKILL_EXPANSION_BUILD_EVIL, SKILL_EXPANSION_BUILD_TRICKS,
};

// Sam's page 2 rows laid out Gardener | Stout | Cook, then Gardener | Stout, then Cook.
static const u8 SamActiveBuilds[SKILL_EXPANSION_NEW_ACTIVE_COUNT] = {
    SKILL_EXPANSION_BUILD_GOOD, SKILL_EXPANSION_BUILD_EVIL, SKILL_EXPANSION_BUILD_TRICKS,
    SKILL_EXPANSION_BUILD_GOOD, SKILL_EXPANSION_BUILD_EVIL, SKILL_EXPANSION_BUILD_TRICKS,
};

// Gimli's page 2 rows laid out Stone | Fury | Tally, then Stone | Tally, then Fury.
static const u8 GimliActiveBuilds[SKILL_EXPANSION_NEW_ACTIVE_COUNT] = {
    SKILL_EXPANSION_BUILD_GOOD, SKILL_EXPANSION_BUILD_EVIL,   SKILL_EXPANSION_BUILD_TRICKS,
    SKILL_EXPANSION_BUILD_GOOD, SKILL_EXPANSION_BUILD_TRICKS, SKILL_EXPANSION_BUILD_EVIL,
};

// Aragorn's page 2 rows laid out Ranger | Dead | King, then Ranger | King.
static const u8 AragornActiveBuilds[SKILL_EXPANSION_NEW_ACTIVE_COUNT] = {
    SKILL_EXPANSION_BUILD_TRICKS, SKILL_EXPANSION_BUILD_DEAD, SKILL_EXPANSION_BUILD_GOOD,
    SKILL_EXPANSION_BUILD_TRICKS, SKILL_EXPANSION_BUILD_GOOD,
};

// Saruman's page 2 rows laid out Curunir | White Hand | Sharkey, then Curunir | Sharkey, then White
// Hand.
static const u8 SarumanActiveBuilds[SKILL_EXPANSION_NEW_ACTIVE_COUNT] = {
    SKILL_EXPANSION_BUILD_GOOD, SKILL_EXPANSION_BUILD_EVIL,   SKILL_EXPANSION_BUILD_TRICKS,
    SKILL_EXPANSION_BUILD_GOOD, SKILL_EXPANSION_BUILD_TRICKS, SKILL_EXPANSION_BUILD_EVIL,
};

// heroes.json id
#define HERO_ID_SARUMAN 9

u8 skillExpansion_activeBuild(u32 heroId, u32 row)
{
    if (row < SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW || row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT)
    {
        return SKILL_EXPANSION_BUILD_NONE;
    }
    if (heroId == HERO_ID_FRODO)
    {
        return FrodoActiveBuilds[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW];
    }
    if (heroId == HERO_ID_ARAGORN)
    {
        return AragornActiveBuilds[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW];
    }
    if (heroId == HERO_ID_SMEAGOL)
    {
        return SmeagolActiveBuilds[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW];
    }
    if (heroId == HERO_ID_SAM)
    {
        return SamActiveBuilds[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW];
    }
    if (heroId == HERO_ID_GIMLI)
    {
        return GimliActiveBuilds[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW];
    }
    if (heroId == HERO_ID_GANDALF)
    {
        // Gandalf's rows show the form he is in.
        return skillExpansion_gandalfIsWhite() ? SKILL_EXPANSION_BUILD_GOOD
                                               : SKILL_EXPANSION_BUILD_GREY;
    }
    if (heroId == HERO_ID_LEGOLAS)
    {
        // Every row is a Quiver arrow. The nocked one shows gold until it is loosed.
        return skillExpansion_legolasNockedRow() == row ? SKILL_EXPANSION_BUILD_GOOD
                                                        : SKILL_EXPANSION_BUILD_TRICKS;
    }
    if (heroId == HERO_ID_SARUMAN)
    {
        return SarumanActiveBuilds[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW];
    }
    return SKILL_EXPANSION_BUILD_NONE;
}

u8 skillExpansion_passiveBuild(u32 passiveSkillId)
{
    switch (passiveSkillId)
    {
    case SKILL_EXPANSION_PASSIVE_STRENGTH_OF_WILL:
    case SKILL_EXPANSION_PASSIVE_STING_GLOWS_BLUE:
    case SKILL_EXPANSION_PASSIVE_UNWAVERING:
    case SKILL_EXPANSION_PASSIVE_KINGS_PRESENCE:
    case SKILL_EXPANSION_PASSIVE_BLADE_REFORGED:
    case SKILL_EXPANSION_PASSIVE_HANDS_OF_THE_KING:
    case SKILL_EXPANSION_PASSIVE_POOR_SMEAGOL:
    case SKILL_EXPANSION_PASSIVE_NICE_MASTER:
    case SKILL_EXPANSION_PASSIVE_SMEAGOL_IS_FREE:
    case SKILL_EXPANSION_PASSIVE_GREEN_THUMB:
    case SKILL_EXPANSION_PASSIVE_GAFFERS_LORE:
    case SKILL_EXPANSION_PASSIVE_MASTER_GARDENER:
    case SKILL_EXPANSION_PASSIVE_LORE_OF_COLOURS:
    case SKILL_EXPANSION_PASSIVE_RING_LORE:
    case SKILL_EXPANSION_PASSIVE_HEAD_OF_THE_COUNCIL:
    case SKILL_EXPANSION_PASSIVE_DWARVEN_MAIL:
    case SKILL_EXPANSION_PASSIVE_NOT_WITHOUT_ME:
    case SKILL_EXPANSION_PASSIVE_STUBBORN_AS_STONE:
    case SKILL_EXPANSION_PASSIVE_ROBES_OF_WHITE:
    case SKILL_EXPANSION_PASSIVE_UNBENDING:
    case SKILL_EXPANSION_PASSIVE_WHITE_RIDER:
    case SKILL_EXPANSION_PASSIVE_ELF_SIGHT:
    case SKILL_EXPANSION_PASSIVE_STEADY_HAND:
    case SKILL_EXPANSION_PASSIVE_UNERRING:
        return SKILL_EXPANSION_BUILD_GOOD;
    case SKILL_EXPANSION_PASSIVE_WEIGHT_OF_THE_RING:
    case SKILL_EXPANSION_PASSIVE_RINGS_HUNGER:
    case SKILL_EXPANSION_PASSIVE_CLAIM_IT:
    case SKILL_EXPANSION_PASSIVE_HUNGRY:
    case SKILL_EXPANSION_PASSIVE_RAT_CATCHER:
    case SKILL_EXPANSION_PASSIVE_WE_WANTS_IT:
    case SKILL_EXPANSION_PASSIVE_STUBBORN:
    case SKILL_EXPANSION_PASSIVE_SHIRE_GRIT:
    case SKILL_EXPANSION_PASSIVE_I_CAN_CARRY_YOU:
    case SKILL_EXPANSION_PASSIVE_FIRES_OF_INDUSTRY:
    case SKILL_EXPANSION_PASSIVE_MACHINE_OF_WAR:
    case SKILL_EXPANSION_PASSIVE_POWDER_OF_ORTHANC:
    case SKILL_EXPANSION_PASSIVE_GRUDGEBEARER:
    case SKILL_EXPANSION_PASSIVE_BLOOD_ON_THE_AXE:
    case SKILL_EXPANSION_PASSIVE_DURINS_FOLK:
        return SKILL_EXPANSION_BUILD_EVIL;
    case SKILL_EXPANSION_PASSIVE_RIDDLES_IN_THE_DARK:
    case SKILL_EXPANSION_PASSIVE_TRICKSY_HOBBIT:
    case SKILL_EXPANSION_PASSIVE_PRECIOUS_LIES:
    case SKILL_EXPANSION_PASSIVE_STRIDERS_RIPOSTE:
    case SKILL_EXPANSION_PASSIVE_HUNTERS_PATIENCE:
    case SKILL_EXPANSION_PASSIVE_TRACK_THE_QUARRY:
    case SKILL_EXPANSION_PASSIVE_SNIFF_IT_OUT:
    case SKILL_EXPANSION_PASSIVE_FINDERS_KEEPERS:
    case SKILL_EXPANSION_PASSIVE_MY_PRECIOUS:
    case SKILL_EXPANSION_PASSIVE_LEMBAS_CRUMBS:
    case SKILL_EXPANSION_PASSIVE_FINE_SEASONING:
    case SKILL_EXPANSION_PASSIVE_TATERS_FOR_ALL:
    case SKILL_EXPANSION_PASSIVE_CUTPURSE:
    case SKILL_EXPANSION_PASSIVE_SPITE:
    case SKILL_EXPANSION_PASSIVE_PARTING_SHOT:
    case SKILL_EXPANSION_PASSIVE_FRIENDLY_WAGER:
    case SKILL_EXPANSION_PASSIVE_ORC_COUNT:
    case SKILL_EXPANSION_PASSIVE_FORTY_TWO:
    case SKILL_EXPANSION_PASSIVE_SWIFT_NOCK:
    case SKILL_EXPANSION_PASSIVE_FLETCHER:
    case SKILL_EXPANSION_PASSIVE_TWIN_NOCK:
        return SKILL_EXPANSION_BUILD_TRICKS;
    case SKILL_EXPANSION_PASSIVE_RESTLESS_DEAD:
    case SKILL_EXPANSION_PASSIVE_THE_DEAD_RIDE:
    case SKILL_EXPANSION_PASSIVE_LORD_OF_THE_DEAD:
    case SKILL_EXPANSION_PASSIVE_ELVEN_CLOAK:
    case SKILL_EXPANSION_PASSIVE_POINT_BLANK:
    case SKILL_EXPANSION_PASSIVE_UNSHAKEN:
        return SKILL_EXPANSION_BUILD_DEAD;
    case SKILL_EXPANSION_PASSIVE_WANDERING_WIZARD:
    case SKILL_EXPANSION_PASSIVE_OLD_TRICKS:
    case SKILL_EXPANSION_PASSIVE_THE_FALL:
        return SKILL_EXPANSION_BUILD_GREY;
    case SKILL_EXPANSION_PASSIVE_KINDLER_OF_HEARTS:
    case SKILL_EXPANSION_PASSIVE_QUICKENED:
    case SKILL_EXPANSION_PASSIVE_SECRET_FIRE:
        return SKILL_EXPANSION_BUILD_FIRE;
    default:
        return SKILL_EXPANSION_BUILD_NONE;
    }
}
