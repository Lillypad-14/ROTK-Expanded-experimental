// Aragorn's Two Towers actives.

#include "patches/legacy_skills/legacy_fx.h"

#if PC_EDITION && SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

#include "player.h"
#include "sfx.h"
#include "stats.h"

// Herbal Remedy (Two Towers 0x0803C4A4, 0x0801D6A0): heals 25 + 5 per level for 8 + 2 per level
// Spirit, with Magical Healing's sparkle on Aragorn.
static const LegacyHealDef HerbalRemedy = {
    .hp = {25, 5},
    .sparkle = 0x080501D8u,
    .sound = SFX_HERB_PICKUP,
};

static void castHerbalRemedy(Actor *playerActor)
{
    legacyFx_castHeal(playerActor, &HerbalRemedy);
}

// Commanding Presence (Two Towers 0x0803B754, the shout 0x08009F60, its tick 0x0800A0C8 and hit
// 0x08011108): for 7 Spirit (the description says +2 per level), a shout on Aragorn that stuns the
// foes within 75 pixels on its first tick for 30 + 6 per level ticks, as Gimli's Battle Cry.
static const LegacyBurstDef CommandingPresence = {
    .animation = 0x0804E1E8u,
    .radius = 75,
    .lifetime = 30,
    .stunTicks = {30, 6},
    .sound = LEGACY_SOUND_NONE,
    .flags = LEGACY_BURST_FOLLOW | LEGACY_BURST_FIRST_TICK | LEGACY_BURST_UNTIL_ANIMATION_END,
};

static void castCommandingPresence(Actor *playerActor)
{
    legacyFx_castBurst(playerActor, &CommandingPresence);
}

// Rage of the North (Two Towers 0x0803B7B4, the rage 0x0800A390, its tick 0x0800A514): for 10 Spirit
// and 3 corruption per level, one rage at a time for 450 ticks: +20% speed and +2 damage per level.
#define RAGE_CORRUPTION_PER_LEVEL 3

static Actor *rages[2];

static const LegacyBuffDef Rage = {
    .rising = 0x0804E208u,
    .burning = 0x0804E218u,
    .fading = 0x0804E228u,
    .ticks = {450, 0},
    .stats = {
        {STAT_SPEED_PERCENT, {20, 0}},
        {STAT_DAMAGE_FLAT, {0, 2}},
    },
    .active = rages,
};

static bool32 rageCanCast(u8 playerIndex)
{
    return legacyFx_buffCanCast(&Rage, playerIndex);
}

static void castRageOfTheNorth(Actor *playerActor)
{
    player_addCorruption(playerActor->as.combat.playerIndex,
                         RAGE_CORRUPTION_PER_LEVEL * legacyFx_castingLevel(playerActor));
    legacyFx_castBuff(playerActor, &Rage);
}

// Sword Throw (Two Towers 0x0803B6FC, the sword 0x0800A950, its hit 0x08011004): for 4 Spirit, the
// sword flies 21 ticks and hits every foe it passes with Aragorn's own melee hit. The hit adds
// nothing per level (the description says +1). Its two trail lights are Gimli's axe's.
static const u32 SwordSprites[8] = {
    0x08327C64u, 0x08327C6Cu, 0x08327C74u, 0x08327C7Cu,
    0x08327C84u, 0x08327C7Cu, 0x08327C74u, 0x08327C6Cu,
};

static const LegacyThrownDef Sword = {
    .sprites = SwordSprites,
    .speed = 0x50000,
    .lifetime = 21,
    .sound = LEGACY_SOUND_NONE,
    .trail = {LEGACY_TRAIL_ORANGE, LEGACY_TRAIL_YELLOW},
};

static void castSwordThrow(Actor *playerActor)
{
    legacyFx_castThrown(playerActor, &Sword);
}

// Sweep (Two Towers 0x0803B6A4, 0x0801D560, its spin 0x0803E34C): for 8 Spirit, Aragorn spins and
// every foe his swing hits takes his melee damage +1 per level. It is ROTK's Sweep, whose hits add
// the stored damage to the melee hit, with the stored damage The Two Towers' level.
static void sweep(Actor *playerActor)
{
    skill_active_aragornSweep(playerActor);
}

static void castSweep(Actor *playerActor)
{
    skillExpansion_castStockCallback(playerActor);
    playerActor->as.combat.storedDamage = legacyFx_castingLevel(playerActor);
}

// Call Legolas (Two Towers 0x0803B81C, 0x0801D60C): for 24 Spirit, Legolas fights beside Aragorn for
// 300 + 60 per level ticks, shooting at foes within 70 pixels.
static const u32 LegolasWalk[5] = {0x08328B34u, 0x08328B3Cu, 0x08328B44u, 0x08328B4Cu, 0x08328B54u};
static const u32 LegolasIdle[5] = {0x08328B84u, 0x08328B8Cu, 0x08328B94u, 0x08328B9Cu, 0x08328BA4u};
static const u32 LegolasAttack[5] = {0x08328BACu, 0x08328BB4u, 0x08328BBCu, 0x08328BC4u, 0x08328BCCu};
static const u32 BowWalk[5] = {0x0832FD08u, 0x0832FD10u, 0x0832FD18u, 0x0832FD20u, 0x0832FD28u};
static const u32 BowIdle[5] = {0x0832FD58u, 0x0832FD60u, 0x0832FD68u, 0x0832FD70u, 0x0832FD78u};
static const u32 BowAttack[5] = {0x0832FD80u, 0x0832FD88u, 0x0832FD90u, 0x0832FD98u, 0x0832FDA0u};

static const LegacyAllyDef Legolas = {
    .walk = LegolasWalk,
    .idle = LegolasIdle,
    .attack = LegolasAttack,
    .weaponWalk = BowWalk,
    .weaponIdle = BowIdle,
    .weaponAttack = BowAttack,
    .hit = LEGACY_ALLY_HIT_ARROW,
    .range = 70,
    .paletteId = 24,
    .lifetime = {300, 60},
};

static void castCallLegolas(Actor *playerActor)
{
    legacyFx_castAlly(playerActor, &Legolas);
}

// ROTK's rows the actives take their cast poses from
static const SkillExpansionActiveDef SelfRow = {.baseRow = ACTIVE_SKILL_ARAGORN_NUMENOREAN_WILL};
static const SkillExpansionActiveDef ShoutRow = {.baseRow = ACTIVE_SKILL_ARAGORN_KINGS_COMMAND};
static const SkillExpansionActiveDef RageRow = {
    .baseRow = ACTIVE_SKILL_ARAGORN_NUMENOREAN_WILL,
    .canCast = rageCanCast,
};
static const SkillExpansionActiveDef CallRow = {
    .baseRow = ACTIVE_SKILL_ARAGORN_KINGS_COMMAND,
    .canCast = legacyFx_allyCanCast,
};
static const SkillExpansionActiveDef SwordThrowRow = {.baseRow = ACTIVE_SKILL_ARAGORN_SWORD_THROW};
static const SkillExpansionActiveDef SweepRow = {
    .stockCallback = sweep,
    .baseRow = ACTIVE_SKILL_ARAGORN_SWEEP,
};

const LegacyActive LegacyAragornActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT] = {
    [0] = {&SelfRow, 8, castHerbalRemedy, 2},
    [1] = {&ShoutRow, 7, castCommandingPresence},
    [2] = {&RageRow, 10, castRageOfTheNorth},
    [3] = {&CallRow, 24, castCallLegolas},
    [4] = {&SwordThrowRow, 4, castSwordThrow},
    [5] = {&SweepRow, 8, castSweep},
};

#endif
