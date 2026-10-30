// Eowyn's Two Towers actives.

#include "patches/legacy_skills/legacy_fx.h"

#if PC_EDITION && SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

#include "player.h"
#include "sfx.h"
#include "stats.h"

// Herbal Remedy (Two Towers 0x0803C5E0, 0x0801EE54): heals 25 + 5 per level for 8 + 2 per level
// Spirit, with Magical Healing's sparkle on Eowyn.
static const LegacyHealDef HerbalRemedy = {
    .hp = {25, 5},
    .sparkle = 0x080501D8u,
    .sound = SFX_HERB_PICKUP,
};

static void castHerbalRemedy(Actor *playerActor)
{
    legacyFx_castHeal(playerActor, &HerbalRemedy);
}

// Defending Fury (Two Towers 0x0803BFA8, the fury 0x08013E4C, its tick 0x08013FB0): for 10 Spirit
// and 3 corruption per level, one at a time: +10 Defense +2 per level for 150 + 15 per level ticks.
#define DEFENDING_FURY_CORRUPTION_PER_LEVEL 3

static Actor *defendingFuries[2];

static const LegacyBuffDef DefendingFury = {
    .rising = 0x0804FF7Cu,
    .burning = 0x0804FF8Cu,
    .fading = 0x0804FF9Cu,
    .ticks = {150, 15},
    .stats = {{STAT_DEFENSE, {10, 2}}},
    .active = defendingFuries,
};

static bool32 defendingFuryCanCast(u8 playerIndex)
{
    return legacyFx_buffCanCast(&DefendingFury, playerIndex);
}

static void castDefendingFury(Actor *playerActor)
{
    player_addCorruption(playerActor->as.combat.playerIndex,
                         DEFENDING_FURY_CORRUPTION_PER_LEVEL * legacyFx_castingLevel(playerActor));
    legacyFx_castBuff(playerActor, &DefendingFury);
}

// Princess of Rohan (Two Towers 0x0803BE88, the cry 0x080140B4, its tick 0x0801421C and hit
// 0x0801166C): for 4 Spirit (the description says 4 per level), a cry on Eowyn that stuns the foes
// within 75 pixels on its first tick for 30 + 6 per level ticks, as Gimli's Battle Cry.
static const LegacyBurstDef PrincessOfRohan = {
    .animation = 0x0804FFACu,
    .radius = 75,
    .lifetime = 30,
    .stunTicks = {30, 6},
    .sound = LEGACY_SOUND_NONE,
    .flags = LEGACY_BURST_FOLLOW | LEGACY_BURST_FIRST_TICK | LEGACY_BURST_UNTIL_ANIMATION_END,
};

static void castPrincessOfRohan(Actor *playerActor)
{
    legacyFx_castBurst(playerActor, &PrincessOfRohan);
}

// Shield Bash (Two Towers 0x0803BEE8, 0x0801EF68, its hits 0x080109D4 and 0x080102B4): for 8 Spirit,
// Eowyn bashes with her shield for 5 + 1 per level and a stun. It is ROTK's Shield Bash, whose hit
// does the stored damage, with the stored damage The Two Towers' and ROTK's stun.
#define SHIELD_BASH_DAMAGE_BASE 5

static void shieldBash(Actor *playerActor)
{
    skill_active_eowynShieldBash(playerActor);
}

static void castShieldBash(Actor *playerActor)
{
    skillExpansion_castStockCallback(playerActor);
    playerActor->as.combat.storedDamage = SHIELD_BASH_DAMAGE_BASE + legacyFx_castingLevel(playerActor);
}

// Double Strike (Two Towers 0x0803BF4C, 0x0801F0D0, its strike 0x08013AA8): for 7 Spirit, a fast
// attack whose hits do +4 + 1 per level. It is ROTK's Double Strike, whose hits add the stored damage
// to the melee hit, with the stored damage The Two Towers' bonus.
#define DOUBLE_STRIKE_DAMAGE_BASE 4

static void doubleStrike(Actor *playerActor)
{
    skill_active_eowynDoubleStrike(playerActor);
}

static void castDoubleStrike(Actor *playerActor)
{
    skillExpansion_castStockCallback(playerActor);
    playerActor->as.combat.storedDamage = DOUBLE_STRIKE_DAMAGE_BASE + legacyFx_castingLevel(playerActor);
}

// ROTK's rows the actives take their cast poses from
static const SkillExpansionActiveDef SelfRow = {.baseRow = ACTIVE_SKILL_EOWYN_SHIELDMAIDEN_OF_ROHAN};
static const SkillExpansionActiveDef DefendingFuryRow = {
    .baseRow = ACTIVE_SKILL_EOWYN_SHIELDMAIDEN_OF_ROHAN,
    .canCast = defendingFuryCanCast,
};
static const SkillExpansionActiveDef CryRow = {.baseRow = ACTIVE_SKILL_EOWYN_FORTH_EORLINGAS};
// a stand-in for Call Eomer: ROTK's Forth Eorlingas
static const SkillExpansionActiveDef ForthEorlingas = {
    .stockCallback = skill_active_eowynForthEorlingas,
    .baseRow = ACTIVE_SKILL_EOWYN_FORTH_EORLINGAS,
};
static const SkillExpansionActiveDef ShieldBashRow = {
    .stockCallback = shieldBash,
    .baseRow = ACTIVE_SKILL_EOWYN_SHIELD_BASH,
};
static const SkillExpansionActiveDef DoubleStrikeRow = {
    .stockCallback = doubleStrike,
    .baseRow = ACTIVE_SKILL_EOWYN_DOUBLE_STRIKE,
};

const LegacyActive LegacyEowynActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT] = {
    {&SelfRow, 8, castHerbalRemedy, 2},
    {&DefendingFuryRow, 10, castDefendingFury},
    {&CryRow, 4, castPrincessOfRohan},
    {&ForthEorlingas, 22}, // Call Eomer
    {&ShieldBashRow, 8, castShieldBash},
    {&DoubleStrikeRow, 7, castDoubleStrike},
};

#endif
