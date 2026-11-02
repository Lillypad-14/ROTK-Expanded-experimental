// Gimli's Two Towers actives.

#include "patches/legacy_skills/legacy_fx.h"

#if PC_EDITION && SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

#include "player.h"
#include "sfx.h"
#include "stats.h"
#include "variables.h"

// Axe Throw (Two Towers 0x0803C590, the axe 0x08019EEC, its tick 0x0801A15C and hit 0x08010884):
// for 4 Spirit (the description says 3), one axe at a time that flies for 21 ticks and hits every
// foe it passes with Gimli's own melee hit, +1 damage per level. It stops at a foe that does not
// flinch. Its two trail lights (emitters 2/3 and 2/2) drop orange and yellow sparkles.
static const u32 AxeSprites[8] = {
    0x083282D8u, 0x083282E0u, 0x083282E8u, 0x083282F0u,
    0x083282F8u, 0x083282F0u, 0x083282E8u, 0x083282E0u,
};

static const LegacyThrownDef Axe = {
    .sprites = AxeSprites,
    .speed = 0x50000,
    .lifetime = 21,
    .bonus = {0, 1},
    .sound = LEGACY_SOUND_NONE,
    .trail = {LEGACY_TRAIL_ORANGE, LEGACY_TRAIL_YELLOW},
};

static Actor *axes[2];

static bool32 axeCanCast(u8 playerIndex)
{
    return !legacyFx_thrownAlive(axes[playerIndex], &Axe);
}

static void castAxeThrow(Actor *playerActor)
{
    axes[playerActor->as.combat.playerIndex] = legacyFx_castThrown(playerActor, &Axe);
}

// Whirlwind Attack (Two Towers 0x0803C138, 0x0801ED24, its spin 0x080445E4): for 3 Spirit (the
// description says 6), Gimli spins and every foe his swing hits takes his melee damage +1 per level.
// It is ROTK's Whirling Attack, whose hits add the stored damage to the melee hit, with the stored
// damage The Two Towers' level.
static void whirlingAttack(Actor *playerActor)
{
    skill_active_gimliWhirlingAttack(playerActor);
}

static void castWhirlwind(Actor *playerActor)
{
    skillExpansion_castStockCallback(playerActor);
    playerActor->as.combat.storedDamage = legacyFx_castingLevel(playerActor);
}

// Berserk Fury (Two Towers 0x0803C0D0, the fury 0x08019C20, its tick 0x08019DC0): for 8 Spirit (the
// description says 10) and 3 corruption per level, one fury at a time on Gimli for 300 ticks:
// +20% speed, +10 damage per level and +1 melee armor per 2 levels. The description says +1
// toughness and damage per level.
#define FURY_CORRUPTION_PER_LEVEL 3

static Actor *furies[2];

static const LegacyBuffDef Fury = {
    .rising = 0x08050208u,
    .burning = 0x08050218u,
    .fading = 0x08050228u,
    .ticks = {300, 0},
    .stats = {
        {STAT_SPEED_PERCENT, {20, 0}},
        {STAT_DAMAGE_FLAT, {0, 10}},
        {STAT_MELEE_ARMOR, {0, 1}, 2},
    },
    .active = furies,
};

static bool32 furyCanCast(u8 playerIndex)
{
    return legacyFx_buffCanCast(&Fury, playerIndex);
}

static void castBerserkFury(Actor *playerActor)
{
    player_addCorruption(playerActor->as.combat.playerIndex,
                         FURY_CORRUPTION_PER_LEVEL * legacyFx_castingLevel(playerActor));
    legacyFx_castBuff(playerActor, &Fury);
}

// Healing Salves (Two Towers 0x0803C544, 0x0801EADC): heals 25 + 7 per level for 8 + 2 per level
// Spirit, with a sparkle on Gimli (Magical Healing's).
static const LegacyHealDef HealingSalves = {
    .hp = {25, 7},
    .sparkle = 0x080501D8u,
    .sound = SFX_HERB_PICKUP,
};

static void castHealingSalves(Actor *playerActor)
{
    legacyFx_castHeal(playerActor, &HealingSalves);
}

// Battle Cry (Two Towers 0x0803C070, the cry 0x080197F0, its tick 0x08019958 and hit 0x08011634):
// for 12 Spirit (the description says +1 per level), a cry on Gimli that stuns the foes within 75
// pixels on its first tick for 30 + 6 per level ticks, and lasts as long as its animation.
static const LegacyBurstDef BattleCry = {
    .animation = 0x080501E8u,
    .radius = 75,
    .lifetime = 30,
    .stunTicks = {30, 6},
    .sound = LEGACY_SOUND_NONE,
    .flags = LEGACY_BURST_FOLLOW | LEGACY_BURST_FIRST_TICK | LEGACY_BURST_UNTIL_ANIMATION_END,
};

static void castBattleCry(Actor *playerActor)
{
    legacyFx_castBurst(playerActor, &BattleCry);
}

// ROTK's rows the actives take their cast poses from
static const SkillExpansionActiveDef AxeThrowRow = {
    .baseRow = ACTIVE_SKILL_GIMLI_AXE_THROW,
    .canCast = axeCanCast,
};
static const SkillExpansionActiveDef WhirlingAttackRow = {
    .stockCallback = whirlingAttack,
    .baseRow = ACTIVE_SKILL_GIMLI_WHIRLING_ATTACK,
};
static const SkillExpansionActiveDef FuryRow = {
    .baseRow = ACTIVE_SKILL_GIMLI_DWARVEN_RAGE,
    .canCast = furyCanCast,
};
static const SkillExpansionActiveDef SelfRow = {.baseRow = ACTIVE_SKILL_GIMLI_STOICISM};
static const SkillExpansionActiveDef StompRow = {.baseRow = ACTIVE_SKILL_GIMLI_EARTH_SHATTER};

const LegacyActive LegacyGimliActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT] = {
    {&AxeThrowRow, 4, castAxeThrow},
    {&WhirlingAttackRow, 3, castWhirlwind},
    {&FuryRow, 8, castBerserkFury},
    {&SelfRow, 8, castHealingSalves, 2},
    {&StompRow, 12, castBattleCry},
};

#endif
