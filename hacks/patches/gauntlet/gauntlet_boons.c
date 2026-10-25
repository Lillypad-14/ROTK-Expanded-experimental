#include "patches/gauntlet/gauntlet_boons.h"

#include "enemy.h"
#include "lib/combat_source.h"
#include "lib/fx_kit.h"
#include "lib/status.h"
#include "lib/targeting.h"
#include "patches/gauntlet/gauntlet.h"
#include "patches/gauntlet/gauntlet_boon_effects.h"
#include "patches/gauntlet/gauntlet_level.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "player.h"
#include "rng.h"
#include "stats.h"
#include "variables.h"

typedef struct GauntletBoon
{
    const char *name;
    // Shown on the pick screen, wrapped to two or three lines.
    const char *description;
    u8 rarity;
    u8 parentA;
    u8 parentB;
    u8 heroPlusOne;
} GauntletBoon;

static const GauntletBoon Boons[GAUNTLET_BOON_COUNT] = {
    [GAUNTLET_BOON_EDGE_OF_ANDURIL] = {"Edge of Anduril", "All your damage is increased by 15%.",
                                       RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_DEEP_BREATH] = {"Deep Breath", "Each second: regain 3% max Spirit +1.",
                                   RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_HOBBIT_LUCK] = {"Hobbit Luck",
                                   "Each enemy hit has a 10% chance to deal no damage.",
                                   RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_CHAIN_LIGHTNING] =
        {"Chain Lightning",
         "Your hits have a 15% chance to arc to 2 nearby foes for half "
         "damage.",
         RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_BLOODTHIRST] = {"Bloodthirst", "Each kill: heal 2% max HP +1.", RARITY_RARE, 0,
                                   0, 0},
    [GAUNTLET_BOON_EXECUTIONER] = {"Executioner", "Triple damage to foes below 25% of their HP.",
                                   RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_CORPSE_BLAST] =
        {"Corpse Blast", "Slain foes explode, dealing half their max HP to foes around them.",
         RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_SECOND_WIND] = {"Second Wind",
                                   "Once per room, below 20% HP: heal 40% and stun nearby foes.",
                                   RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_BLOOD_PRICE] = {"Blood Price", "Deal 25% more damage while at full HP.",
                                   RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_HASTE_LOOP] = {"Haste Loop",
                                  "Every kill cuts 1 second off your skill cooldowns.", RARITY_EPIC,
                                  0, 0, 0},
    [GAUNTLET_BOON_LAST_STAND] = {"Last Stand",
                                  "Once per room, a killing blow leaves you at 1 HP, invulnerable "
                                  "with triple damage for 5s.",
                                  RARITY_LEGENDARY, 0, 0, 0},
    [GAUNTLET_BOON_EYE_OF_SAURON] = {"Eye of Sauron",
                                     "Every 10s: hit the healthiest foe for 25% of its max HP +10.",
                                     RARITY_LEGENDARY, 0, 0, 0},
    [GAUNTLET_BOON_HEARTY] = {"Hearty", "Your health grows: about 20% more maximum HP.",
                              RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_IRON_SKIN] = {"Iron Skin", "Blows that hit you deal 10% less damage.",
                                 RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_QUICK_STUDY] = {"Quick Study", "Gain 30% more experience.", RARITY_COMMON, 0, 0,
                                   0},
    [GAUNTLET_BOON_SIPHON_MIND] = {"Siphon Mind", "Each kill restores 5% max Spirit +1.",
                                   RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_VERMIN_SLAYER] = {"Vermin Slayer", "Deal 30% more damage to weak foes.",
                                     RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_THORNS] = {"Thorns", "Foes that hit you take 30% of the damage they deal.",
                              RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_STATIC_CHARGE] = {"Static Charge",
                                     "Every 8th hit releases a shockwave that hurts foes around "
                                     "you.",
                                     RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_BATTLE_CRY] = {"Battle Cry",
                                  "Every 20 kills, a cry stuns every foe near you for 2s.",
                                  RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_TROLL_SLAYER] = {"Troll Slayer", "Deal 40% more damage to large foes.",
                                    RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_RIPOSTE] = {"Riposte",
                               "After a foe hits you, your next blow deals double damage.",
                               RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_MOMENTUM] = {"Momentum",
                                "Kills in quick succession add 5% damage each, up to 5 times.",
                                RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_MEDITATION] = {"Meditation",
                                  "After standing still for 2s: heal 2% max HP +1 each second.",
                                  RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_WARRIORS_REST] = {"Warrior's Rest",
                                     "Enter every room with 20% of your HP restored.", RARITY_RARE,
                                     0, 0, 0},
    [GAUNTLET_BOON_GLASS_CANNON] = {"Glass Cannon", "Deal 40% more damage, and take 25% more.",
                                    RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_BIG_GAME] = {"Big Game",
                                "Deal double damage to elites and giants, and 25% less to the "
                                "rest.",
                                RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_BERSERKER] = {"Berserker", "Below half HP, deal 40% more damage.", RARITY_EPIC,
                                 0, 0, 0},
    [GAUNTLET_BOON_AVALANCHE] =
        {"Avalanche", "Hits drop a boulder at 5% chance +1% per cleared room, capped at 25%.",
         RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_LIGHT_OF_EARENDIL] = {"Light of Earendil",
                                         "Foes close to you take 30% more damage from all sources "
                                         "of yours.",
                                         RARITY_LEGENDARY, 0, 0, 0},
    [GAUNTLET_BOON_KINGSFOIL_POUCH] = {"Kingsfoil Pouch",
                                       "Gain 5 herbs now and a herb for every room you enter.",
                                       RARITY_LEGENDARY, 0, 0, 0},
    [GAUNTLET_BOON_SECOND_LIFE] = {"Second Life",
                                   "Once per run, a killing blow instead leaves you at half HP.",
                                   RARITY_LEGENDARY, 0, 0, 0},
    [GAUNTLET_BOON_SWIFT_BLADE] = {"Swift Blade", "Basic attack animations run 15% faster.",
                                   RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_KEEN_EYE] = {"Keen Eye", "Gain 10 percentage points of critical chance.",
                                RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_FLEETFOOT] = {"Fleetfoot", "Gain 12 percentage points of movement speed.",
                                 RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_DWARF_GOLD] = {"Dwarf-gold", "Gain 40% gem find.", RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_STEADY_FOOTING] = {"Steady Footing", "Ordinary damage cannot make you flinch.",
                                      RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_LONG_REACH] = {"Long Reach",
                                  "Shared skill targeting and projectile areas are 20% wider.",
                                  RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_RANGERS_PATIENCE] = {"Ranger's Patience", "Skill cooldowns recover 15% faster.",
                                        RARITY_COMMON, 0, 0, 0},
    [GAUNTLET_BOON_EMBER_TRAIL] = {"Ember Trail",
                                   "While moving, ignite foes within 32px every half second.",
                                   RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_ECHO_CAST] = {"Echo Cast",
                                 "Skills have a 20% chance to release a free spirit echo after "
                                 "0.5s: 10 damage +6 per skill rank.",
                                 RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_OVERKILL] = {"Overkill",
                                "Excess killing damage strikes the nearest foe within 96px.",
                                RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_KINDLING] = {"Kindling", "Hits Burn foes for 3s: 3 damage each second.",
                                RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_BARROW_VENOM] =
        {"Barrow Venom", "Hits add Venom for 3s. Each stack deals 1 damage per second, up to 10.",
         RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_CARADHRAS_CHILL] =
        {"Caradhras Chill", "Hits slow foes by 25% for 3s. Five Frost stacks freeze for 2s.",
         RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_ORC_CLEAVER] = {"Orc-cleaver",
                                   "Hits cause 3s of Bleed: moving foes take 4 damage per second.",
                                   RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_BLACK_SPEECH] =
        {"Black Speech", "Kills Dread nearby foes for 3s. They retreat and take 20% more damage.",
         RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_SHATTER] = {"Shatter",
                               "Frozen foes take double damage and burst for 15 damage on death.",
                               RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_PLAGUE_SPREADER] =
        {"Plague Spreader",
         "Venom on a slain foe jumps to the nearest foe within 96px at half stacks.", RARITY_RARE,
         0, 0, 0},
    [GAUNTLET_BOON_WILDFIRE_SEED] = {"Wildfire Seed",
                                     "Burning foes ignite other foes within 24px once per second.",
                                     RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_GEM_MAGNET] =
        {"Gem Magnet",
         "Gems within 240px move toward you. Each collected gem pile heals 1% max HP.", RARITY_RARE,
         0, 0, 0},
    [GAUNTLET_BOON_WARG_HUNTER] = {"Warg Hunter",
                                   "Deal 30% more damage to animals, wargs and spiders.",
                                   RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_SPLIT_SHOT] =
        {"Split Shot", "Each successful skill fires two extra spirit bolts for 10 damage each.",
         RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_OVERFLOW] =
        {"Overflow",
         "Regain 2% Spirit per second up to 150% capacity. Overfilled casts deal 50% more damage.",
         RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_RITUAL] = {"Ritual",
                              "Every third consecutive cast of the same skill deals triple damage.",
                              RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_SUMMONERS_PACT] =
        {"Summoner's Pact", "Every 15 kills summons a ghost ally for 10s. At most four ghosts.",
         RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_BOOMERANG] =
        {"Boomerang",
         "Projectile hits strike their target again after half a second for half damage.",
         RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_DETONATE] =
        {"Detonate",
         "When your Burn, Venom or Bleed expires, it explodes for 12 damage within 40px.",
         RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_TWIN_SOULS] = {"Twin Souls",
                                  "Each skill also strikes a nearby foe for 6 damage per rank of "
                                  "your weakest learned skill.",
                                  RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_SIPHON] = {"Siphon",
                              "Each skill hit restores 10% of that skill's Spirit cost, minimum 1.",
                              RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_ELEMENTAL_WEAVE] =
        {"Elemental Weave",
         "Casts cycle Burn, Frost and Venom on hits. Third casts burst for 15 extra area damage.",
         RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_GRAVITY_WELL] =
        {"Gravity Well",
         "Skill hits pull nearby foes up to 8px toward the impact, where floor allows.",
         RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_PHALANX] =
        {"Phalanx", "While casting, take 50% less damage from attackers in front of you.",
         RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_MIRROR_IMAGE] = {"Mirror Image",
                                    "Casting creates a decoy ward that absorbs one hit within 2s.",
                                    RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_RAPID_FIRE] =
        {"Rapid Fire", "Hits speed basic attack animations by 5% each for 3s, up to 50%.",
         RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_LAST_BREATH] =
        {"Last Breath",
         "A cast leaving too little Spirit to repeat that skill deals triple damage.", RARITY_EPIC,
         0, 0, 0},
    [GAUNTLET_BOON_WANDERING_STAR] =
        {"Wandering Star", "Each second a star strikes the nearest foe within 128px for 10 damage.",
         RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_THE_ONE_RING] = {"The One Ring",
                                    "After 2s without landing a hit, your next hit deals double "
                                    "damage. Lose 1% max HP on room entry.",
                                    RARITY_LEGENDARY, 0, 0, 0},
    [GAUNTLET_BOON_WRAITH_FORM] =
        {"Wraith Form", "Successful skills grant 1s of damage immunity. Healing is halved.",
         RARITY_LEGENDARY, 0, 0, 0},
    [GAUNTLET_BOON_BALROGS_WHIP] = {"Balrog's Whip",
                                    "Basic melee hits burn and splash nearby foes for half damage.",
                                    RARITY_LEGENDARY, 0, 0, 0},
    [GAUNTLET_BOON_ARMY_OF_THE_DEAD] =
        {"Army of the Dead", "Kills raise ghosts for 60s or until room exit. At most four ghosts.",
         RARITY_LEGENDARY, 0, 0, 0},
    [GAUNTLET_BOON_MITHRANDIRS_STAFF] = {"Mithrandir's Staff",
                                         "Each successful skill clears hostile projectiles within "
                                         "96px and deals 15 light damage nearby.",
                                         RARITY_LEGENDARY, 0, 0, 0},
    [GAUNTLET_BOON_STORM_PYRE] =
        {"Storm Pyre", "Lightning arcs ignite foes. Burning foes explode for 15 damage on death.",
         RARITY_EPIC, GAUNTLET_BOON_CHAIN_LIGHTNING + 1, GAUNTLET_BOON_CORPSE_BLAST + 1, 0},
    [GAUNTLET_BOON_WILDFIRE] = {"Wildfire",
                                "Fire damage kills cut skill cooldowns by an additional second.",
                                RARITY_EPIC, GAUNTLET_BOON_EMBER_TRAIL + 1,
                                GAUNTLET_BOON_HASTE_LOOP + 1, 0},
    [GAUNTLET_BOON_VOLLEY_ECHO] = {"Volley Echo",
                                   "Free echo casts fire four extra spirit bolts instead of two.",
                                   RARITY_EPIC, GAUNTLET_BOON_ECHO_CAST + 1,
                                   GAUNTLET_BOON_SPLIT_SHOT + 1, 0},
    [GAUNTLET_BOON_IRON_HIDE] = {"Iron Hide", "Every 20 thorn reflections recharges Second Wind.",
                                 RARITY_EPIC, GAUNTLET_BOON_THORNS + 1,
                                 GAUNTLET_BOON_SECOND_WIND + 1, 0},
    [GAUNTLET_BOON_PERMAFROST] = {"Permafrost",
                                  "Shatter explosions add five Frost stacks to nearby foes.",
                                  RARITY_EPIC, GAUNTLET_BOON_CARADHRAS_CHILL + 1,
                                  GAUNTLET_BOON_SHATTER + 1, 0},
    [GAUNTLET_BOON_PANDEMIC] = {"Pandemic",
                                "Venom spreads at full stacks on death instead of half stacks.",
                                RARITY_EPIC, GAUNTLET_BOON_BARROW_VENOM + 1,
                                GAUNTLET_BOON_PLAGUE_SPREADER + 1, 0},
    [GAUNTLET_BOON_REAPERS_DUE] =
        {"Reaper's Due", "Excess killing damage spills to a nearby foe at double strength.",
         RARITY_EPIC, GAUNTLET_BOON_EXECUTIONER + 1, GAUNTLET_BOON_OVERKILL + 1, 0},
    [GAUNTLET_BOON_SIEGE_ENGINE] =
        {"Siege Engine",
         "Avalanche boulders target the healthiest nearby foe and deal double damage.", RARITY_EPIC,
         GAUNTLET_BOON_BIG_GAME + 1, GAUNTLET_BOON_AVALANCHE + 1, 0},
    [GAUNTLET_BOON_ELVEN_VOLLEY] = {"Elven Volley",
                                    "Arrow hits also strike up to two nearby foes for half damage.",
                                    RARITY_EPIC, 0, 0, 2},
    [GAUNTLET_BOON_FAULT_LINE] =
        {"Fault Line",
         "Earth Shatter leaves a 3s fissure that deals 6 damage every half second nearby.",
         RARITY_EPIC, 0, 0, 6},
    [GAUNTLET_BOON_YOU_SHALL_NOT_PASS] =
        {"You Shall Not Pass",
         "After casting Shield, reflect hostile projectiles within 48px for 10 damage, for 3s.",
         RARITY_EPIC, 0, 0, 4},
    [GAUNTLET_BOON_SPREADING_DREAD] = {"Spreading Dread", "Kills spread Dread within 80px for 3s.",
                                       RARITY_EPIC, 0, 0, 9},
    [GAUNTLET_BOON_VOICE_OF_COMMAND] =
        {"Voice of Command", "Skill hits echo on their target after half a second for half damage.",
         RARITY_EPIC, 0, 0, 10},
    [GAUNTLET_BOON_HEIR_OF_ISILDUR] = {"Heir of Isildur", "Your ghost summons last twice as long.",
                                       RARITY_EPIC, 0, 0, 3},
    [GAUNTLET_BOON_EMBER_DIVIDEND] = {"Ember Dividend",
        "Aragorn: each time Flame reaches 100, restore 15% max Spirit.", RARITY_RARE, 0, 0, 3},
    [GAUNTLET_BOON_QUIET_RESOLVE] = {"Quiet Resolve",
        "Frodo: Burden reductions remove 50% more, minimum 1 extra.", RARITY_RARE, 0, 0, 1},
    [GAUNTLET_BOON_SEED_BANK] = {"Seed Bank",
        "Sam: harvesting a ripe tater adds 25% growth to your other growing plants.",
        RARITY_RARE, 0, 0, 7},
    [GAUNTLET_BOON_RESERVED_POWER] = {"Reserved Power",
        "Saruman: empowered spells leave 1 charge instead of 0. Failed casts keep charges.",
        RARITY_EPIC, 0, 0, 10},
    [GAUNTLET_BOON_MEASURED_POWER] = {"Measured Power",
        "Skills deal 35% more damage. Successful cast cooldowns are 50% longer, minimum 1.5s.",
        RARITY_EPIC, 0, 0, 0},
    [GAUNTLET_BOON_STEEL_OATH] = {"Steel Oath",
        "Basic weapon attacks deal 40% more damage. Skill damage is reduced by 25%.",
        RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_APPRENTICES_OATH] = {"Apprentice's Oath",
        "Successful casts refund 10% of Spirit paid. After 12 paid casts, refund 30%. Free casts "
        "and herbs do not count. L on doors: goals.", RARITY_RARE, 0, 0, 0},
    [GAUNTLET_BOON_VETERANS_PURSE] = {"Veteran's Purse",
        "Kills grant 1 extra gem. After 30 kills with this boon, kills grant 3 gems instead. "
        "L on doors: goals.", RARITY_RARE, 0, 0, 0},
};

static const char *const RarityNames[] = {"Common", "Rare", "Epic", "Legendary"};

// Offer weights by rarity.
static const u8 RarityWeights[] = {8, 5, 3, 1};

#define TICKS_PER_SECOND 30
#define CHAIN_RADIUS 64
#define CORPSE_RADIUS 48
#define SECOND_WIND_RADIUS 64
#define LAST_STAND_TICKS (5 * TICKS_PER_SECOND)
#define EYE_INTERVAL (10 * TICKS_PER_SECOND)
#define HASTE_LOOP_FRAMES 30

struct GauntletBoonState
{
    // bit per enum GauntletBoonId
    u32 owned[3];
    // ticks left of an active Last Stand
    u16 lastStandTicks;
    u16 eyeTicks;
    // ticks left before the Momentum streak ends
    u16 streakTicks;
    u8 spiritTicks;
    // bit 0: Second Wind spent this room, bit 1: Last Stand spent this room
    u8 spent;
    // hits toward the next Static Charge
    u8 hits;
    // kills toward the next Battle Cry
    u8 kills;
    // Momentum kills in a row
    u8 streak;
    // TRUE after a foe hit the player, until the next blow lands (Riposte)
    u8 riposte;
    // ticks the player has stood still (Meditation)
    u8 still;
    // bit 0: Second Life spent this run
    u8 runSpent;
    u8 apprenticeCasts;
    u8 veteranKills;
};

#define SPENT_SECOND_WIND 1
#define SPENT_LAST_STAND 2

static struct GauntletBoonState Boon;

static s32 maxHp(u8 playerIndex)
{
    return PLAYER_STAT(playerIndex, STAT_MAX_HP);
}

static bool32 owns(u8 boon)
{
    return boon < GAUNTLET_BOON_COUNT && ((Boon.owned[boon / 32] >> (boon % 32)) & 1u);
}

bool32 gauntletBoons_has(u8 boon)
{
    return gauntlet_isRunning() && owns(boon) &&
           (Boons[boon].heroPlusOne == 0 || Boons[boon].heroPlusOne == PLAYER(0).heroId + 1);
}

bool32 gauntletBoons_hasExpanded(void)
{
    return gauntlet_isRunning() &&
           (Boon.owned[1] != 0 || Boon.owned[2] != 0 || owns(GAUNTLET_BOON_CHAIN_LIGHTNING) ||
            owns(GAUNTLET_BOON_CORPSE_BLAST) || owns(GAUNTLET_BOON_THORNS) ||
            owns(GAUNTLET_BOON_STATIC_CHARGE) || owns(GAUNTLET_BOON_AVALANCHE) ||
            owns(GAUNTLET_BOON_EYE_OF_SAURON));
}

u32 gauntletBoons_stacks(u8 boon)
{
    return boon < GAUNTLET_BOON_COUNT ? owns(boon) : 0;
}

void gauntletBoons_reset(void)
{
    Boon = (struct GauntletBoonState){0};
    gauntletBoonEffects_reset();
}

void gauntletBoons_roomStart(void)
{
    Boon.spent = 0;
    gauntletBoonEffects_roomStart();
    Boon.streak = 0;
    Boon.streakTicks = 0;
    Boon.riposte = FALSE;
    if (owns(GAUNTLET_BOON_WARRIORS_REST))
    {
        s32 hp = PLAYER(0).currentHp + maxHp(0) * 20 / 100;

        if (owns(GAUNTLET_BOON_WRAITH_FORM))
        {
            hp = PLAYER(0).currentHp + maxHp(0) * 10 / 100;
        }
        PLAYER(0).currentHp = hp > maxHp(0) ? maxHp(0) : hp;
    }
    if (owns(GAUNTLET_BOON_KINGSFOIL_POUCH))
    {
        gauntlet_addHerbs(1);
    }
}

void gauntletBoons_rechargeSecondWind(void)
{
    Boon.spent &= ~SPENT_SECOND_WIND;
}

u32 gauntletBoons_xpBonusPercent(void)
{
    return owns(GAUNTLET_BOON_QUICK_STUDY) ? 30 : 0;
}

void gauntletBoons_grant(u8 boon)
{
    if (boon >= GAUNTLET_BOON_COUNT)
    {
        return;
    }
    if (owns(boon))
    {
        return;
    }
    Boon.owned[boon / 32] |= 1u << (boon % 32);
    if (boon == GAUNTLET_BOON_HEARTY)
    {
        PLAYER(0).baseStats.health += 5;
        gauntletLevel_refreshStats(0);
    }
    else if (boon == GAUNTLET_BOON_KINGSFOIL_POUCH)
    {
        gauntlet_addHerbs(5);
    }
}

const char *gauntletBoons_name(u8 boon)
{
    return boon < GAUNTLET_BOON_COUNT ? Boons[boon].name : "";
}

const char *gauntletBoons_description(u8 boon)
{
    return boon < GAUNTLET_BOON_COUNT ? Boons[boon].description : "";
}

u32 gauntletBoons_goalProgress(u8 boon)
{
    return boon == GAUNTLET_BOON_APPRENTICES_OATH ? Boon.apprenticeCasts
           : boon == GAUNTLET_BOON_VETERANS_PURSE ? Boon.veteranKills : 0;
}

u32 gauntletBoons_goalTarget(u8 boon)
{
    return boon == GAUNTLET_BOON_APPRENTICES_OATH ? 12
           : boon == GAUNTLET_BOON_VETERANS_PURSE ? 30 : 0;
}

void gauntletBoons_paidCast(u8 playerIndex, s32 paid)
{
    if (playerIndex != 0 || paid <= 0 || !gauntletBoons_has(GAUNTLET_BOON_APPRENTICES_OATH))
    {
        return;
    }
    if (Boon.apprenticeCasts < 12 && ++Boon.apprenticeCasts == 12)
    {
        skillExpansion_feedback(playerIndex, "Apprentice's Oath evolved!");
    }
    s32 cap = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
    if (gauntletBoons_has(GAUNTLET_BOON_OVERFLOW))
    {
        cap = cap * 3 / 2;
    }
    s32 spirit = PLAYER(playerIndex).currentSpirit + paid * (Boon.apprenticeCasts >= 12 ? 30 : 10) / 100;
    if (PLAYER(playerIndex).currentSpirit < cap)
    {
        PLAYER(playerIndex).currentSpirit = spirit > cap ? cap : spirit;
    }
}

void gauntletBoons_flameFilled(u8 playerIndex)
{
    if (playerIndex == 0 && gauntletBoons_has(GAUNTLET_BOON_EMBER_DIVIDEND))
    {
        s32 cap = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
        s32 spirit = PLAYER(playerIndex).currentSpirit + cap * 15 / 100;
        if (PLAYER(playerIndex).currentSpirit < cap)
        {
            PLAYER(playerIndex).currentSpirit = spirit > cap ? cap : spirit;
        }
    }
}

s32 gauntletBoons_burdenChange(u8 playerIndex, s32 amount)
{
    if (playerIndex == 0 && amount < 0 && gauntletBoons_has(GAUNTLET_BOON_QUIET_RESOLVE))
    {
        amount += amount / 2 != 0 ? amount / 2 : -1;
    }
    return amount;
}

// A pair in either direction can guide an offer. Parent requirements still apply.
static const u8 SynergyPairs[][2] = {
    {GAUNTLET_BOON_CHAIN_LIGHTNING, GAUNTLET_BOON_CORPSE_BLAST},
    {GAUNTLET_BOON_EMBER_TRAIL, GAUNTLET_BOON_HASTE_LOOP},
    {GAUNTLET_BOON_ECHO_CAST, GAUNTLET_BOON_SPLIT_SHOT},
    {GAUNTLET_BOON_THORNS, GAUNTLET_BOON_SECOND_WIND},
    {GAUNTLET_BOON_CARADHRAS_CHILL, GAUNTLET_BOON_SHATTER},
    {GAUNTLET_BOON_BARROW_VENOM, GAUNTLET_BOON_PLAGUE_SPREADER},
    {GAUNTLET_BOON_EXECUTIONER, GAUNTLET_BOON_OVERKILL},
    {GAUNTLET_BOON_BIG_GAME, GAUNTLET_BOON_AVALANCHE},
    {GAUNTLET_BOON_MEASURED_POWER, GAUNTLET_BOON_RANGERS_PATIENCE},
    {GAUNTLET_BOON_STEEL_OATH, GAUNTLET_BOON_SWIFT_BLADE},
    {GAUNTLET_BOON_APPRENTICES_OATH, GAUNTLET_BOON_OVERFLOW},
};

bool32 gauntletBoons_isSynergy(u8 boon)
{
    if (boon >= GAUNTLET_BOON_COUNT)
    {
        return FALSE;
    }
    if (Boons[boon].parentA != 0 && Boons[boon].parentB != 0 &&
        owns(Boons[boon].parentA - 1) && owns(Boons[boon].parentB - 1))
    {
        return TRUE;
    }
    for (u32 i = 0; i < ARRAY_COUNT(SynergyPairs); i++)
    {
        if ((SynergyPairs[i][0] == boon && owns(SynergyPairs[i][1])) ||
            (SynergyPairs[i][1] == boon && owns(SynergyPairs[i][0])))
        {
            return TRUE;
        }
    }
    return FALSE;
}

static bool32 eligible(u8 boon)
{
    return (Boons[boon].parentA == 0 || owns(Boons[boon].parentA - 1)) &&
           (Boons[boon].parentB == 0 || owns(Boons[boon].parentB - 1)) &&
           (Boons[boon].heroPlusOne == 0 || Boons[boon].heroPlusOne == PLAYER(0).heroId + 1);
}

void gauntletBoons_grantRandom(u8 rarity)
{
    u32 count = 0;
    u32 pick;
    u32 boon;

    for (boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
    {
        count += Boons[boon].rarity == rarity && !owns(boon) && eligible(boon);
    }
    if (count == 0)
    {
        return;
    }
    pick = gauntlet_randBelow(count);
    for (boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
    {
        if (Boons[boon].rarity == rarity && !owns(boon) && eligible(boon) && pick-- == 0)
        {
            gauntletBoons_grant(boon);
            return;
        }
    }
}

u8 gauntletBoons_rarity(u8 boon)
{
    return boon < GAUNTLET_BOON_COUNT ? Boons[boon].rarity : RARITY_COMMON;
}

const char *gauntletBoons_rarityName(u8 boon)
{
    return boon < GAUNTLET_BOON_COUNT ? RarityNames[Boons[boon].rarity] : "";
}

u32 gauntletBoons_ownedCount(void)
{
    u32 count = 0;
    u32 boon;

    for (boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
    {
        count += owns(boon);
    }
    return count;
}

static bool32 offered(const u8 offers[GAUNTLET_OFFER_COUNT], u32 filled, u8 boon)
{
    u32 i;

    for (i = 0; i < filled; i++)
    {
        if (offers[i] == boon)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void gauntletBoons_roll(u8 offers[GAUNTLET_OFFER_COUNT], u8 minRarity)
{
    u32 filled = 0;
    u32 attempt;
    bool32 legendary = FALSE;
    u8 boon;

    // One in three rolls reserves the first slot for a compatible build choice.
    if (gauntlet_randBelow(3) == 0)
    {
        u32 candidates = 0;
        for (boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
        {
            if (!owns(boon) && eligible(boon) && Boons[boon].rarity >= minRarity &&
                gauntletBoons_isSynergy(boon))
            {
                candidates++;
            }
        }
        if (candidates != 0)
        {
            u32 pick = gauntlet_randBelow(candidates);
            for (boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
            {
                if (!owns(boon) && eligible(boon) && Boons[boon].rarity >= minRarity &&
                    gauntletBoons_isSynergy(boon) && pick-- == 0)
                {
                    offers[filled++] = boon;
                    legendary = Boons[boon].rarity == RARITY_LEGENDARY;
                    break;
                }
            }
        }
    }
    for (attempt = 0; attempt < 64 && filled < GAUNTLET_OFFER_COUNT; attempt++)
    {
        u8 rarity;

        boon = gauntlet_randBelow(GAUNTLET_BOON_COUNT);
        rarity = Boons[boon].rarity;
        if (!eligible(boon) || owns(boon) || offered(offers, filled, boon) || rarity < minRarity ||
            (legendary && rarity == RARITY_LEGENDARY) ||
            gauntlet_randBelow(RarityWeights[RARITY_COMMON]) >= RarityWeights[rarity])
        {
            continue;
        }
        legendary |= rarity == RARITY_LEGENDARY;
        offers[filled++] = boon;
    }
    // Chance can miss when few boons are left, so fill from the list in order.
    for (boon = 0; boon < GAUNTLET_BOON_COUNT && filled < GAUNTLET_OFFER_COUNT; boon++)
    {
        if (eligible(boon) && !owns(boon) && !offered(offers, filled, boon) &&
            Boons[boon].rarity >= minRarity &&
            !(legendary && Boons[boon].rarity == RARITY_LEGENDARY))
        {
            legendary |= Boons[boon].rarity == RARITY_LEGENDARY;
            offers[filled++] = boon;
        }
    }
    while (filled < GAUNTLET_OFFER_COUNT)
    {
        offers[filled++] = GAUNTLET_NO_BOON;
    }
}

u8 gauntletBoons_rollBestAvailable(u8 offers[GAUNTLET_OFFER_COUNT], u8 preferredRarity)
{
    u8 rarity = preferredRarity > RARITY_LEGENDARY ? RARITY_LEGENDARY : preferredRarity;
    for (;;)
    {
        gauntletBoons_roll(offers, rarity);
        if (offers[0] != GAUNTLET_NO_BOON || rarity == RARITY_COMMON)
        {
            return rarity;
        }
        rarity--;
    }
}

static s32 foeMaxHp(Actor *foe)
{
    return combat_getEnemyMaxHp(foe->unitType);
}

/** Boon damage to one foe, unable to trigger other boons. */
static void boonDamage(Actor *owner, Actor *foe, s32 damage)
{
    if (damage < 1)
    {
        damage = 1;
    }
    if (damage > 0xffff)
    {
        damage = 0xffff;
    }
    (void)owner;
    gauntletBoonEffects_queue(foe, damage, HACKLIB_DAMAGE_SECONDARY, 1);
}

static void spiritRegen(u8 playerIndex)
{
    s32 cap = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
    if (owns(GAUNTLET_BOON_OVERFLOW))
    {
        cap = cap * 3 / 2;
    }
    s32 spirit;

    if (++Boon.spiritTicks < TICKS_PER_SECOND)
    {
        return;
    }
    Boon.spiritTicks = 0;
    spirit =
        PLAYER(playerIndex).currentSpirit + PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT) * 3 / 100 + 1;
    PLAYER(playerIndex).currentSpirit = spirit > cap ? cap : spirit;
}

static void secondWind(u8 playerIndex, Actor *owner)
{
    Actor *foe = NULL;

    if ((Boon.spent & SPENT_SECOND_WIND) || PLAYER(playerIndex).currentHp * 5 >= maxHp(playerIndex))
    {
        return;
    }
    Boon.spent |= SPENT_SECOND_WIND;
    skillExpansion_addHp(playerIndex, maxHp(playerIndex) * 40 / 100, FALSE);
    while ((foe = hackLib_nextTargetAt(&owner->coords, owner, SECOND_WIND_RADIUS, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            hackLib_applyStun(foe, TICKS_PER_SECOND);
        }
    }
    hackLib_fxBurst(owner, FX_SHAPE_RING_SPARKLES, FX_COLOR_WHITE, FX_LAYER_FRONT);
    skillExpansion_feedback(playerIndex, "Second Wind!");
}

static void eyeOfSauron(Actor *owner)
{
    Actor *strongest = NULL;
    Actor *cursor;

    if (++Boon.eyeTicks < EYE_INTERVAL)
    {
        return;
    }
    Boon.eyeTicks = 0;
    for (cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
    {
        if (skillExpansion_isFoe(cursor) &&
            (strongest == NULL || cursor->currentHp > strongest->currentHp))
        {
            strongest = cursor;
        }
    }
    if (strongest != NULL)
    {
        hackLib_fxBurst(strongest, FX_SHAPE_LIGHTNING, FX_COLOR_EMBER, FX_LAYER_FRONT);
        boonDamage(owner, strongest, foeMaxHp(strongest) / 4 + 10);
    }
}

/** Standing still for two seconds heals a little every second after. */
static void meditate(u8 playerIndex, Actor *owner)
{
    if (owner->velocity.x != 0 || owner->velocity.y != 0)
    {
        Boon.still = 0;
        return;
    }
    if (Boon.still < 255)
    {
        Boon.still++;
    }
    if (Boon.still >= 2 * TICKS_PER_SECOND + TICKS_PER_SECOND)
    {
        Boon.still = 2 * TICKS_PER_SECOND;
        skillExpansion_addHp(playerIndex, maxHp(playerIndex) * 2 / 100 + 1, FALSE);
        hackLib_fxBurst(owner, FX_SHAPE_SPARKLE_RING, FX_COLOR_GREEN, FX_LAYER_FRONT);
    }
}

void gauntletBoons_tick(u8 playerIndex, Actor *owner, u32 room)
{
    (void)room;
    gauntletBoonEffects_tick(playerIndex, owner);
    if (owns(GAUNTLET_BOON_DEEP_BREATH))
    {
        spiritRegen(playerIndex);
    }
    if (owns(GAUNTLET_BOON_SECOND_WIND))
    {
        secondWind(playerIndex, owner);
    }
    if (owns(GAUNTLET_BOON_EYE_OF_SAURON))
    {
        eyeOfSauron(owner);
    }
    if (Boon.lastStandTicks != 0 && --Boon.lastStandTicks == 0)
    {
        owner->flags.p &= ~ACTOR_FLAG_INVULNERABLE;
    }
    if (Boon.streakTicks != 0 && --Boon.streakTicks == 0)
    {
        Boon.streak = 0;
    }
    if (owns(GAUNTLET_BOON_MEDITATION))
    {
        meditate(playerIndex, owner);
    }
}

#define WEAK_FOE_HP 50
#define LARGE_FOE_HP 120
#define ELITE_FOE_HP 150
#define LIGHT_RADIUS 72

/** The newer boons' damage multipliers. */
static s32 scaleDamage(u8 playerIndex, Actor *victim, s32 damage)
{
    s32 foeHp = victim != NULL ? foeMaxHp(victim) : 0;

    if (owns(GAUNTLET_BOON_VERMIN_SLAYER) && foeHp != 0 && foeHp <= WEAK_FOE_HP)
    {
        damage = damage * 130 / 100;
    }
    if (owns(GAUNTLET_BOON_TROLL_SLAYER) && foeHp >= LARGE_FOE_HP)
    {
        damage = damage * 140 / 100;
    }
    if (owns(GAUNTLET_BOON_BIG_GAME) && foeHp != 0)
    {
        damage = foeHp >= ELITE_FOE_HP ? damage * 2 : damage * 75 / 100;
    }
    if (owns(GAUNTLET_BOON_GLASS_CANNON))
    {
        damage = damage * 140 / 100;
    }
    if (owns(GAUNTLET_BOON_BERSERKER) && PLAYER(playerIndex).currentHp * 2 < maxHp(playerIndex))
    {
        damage = damage * 140 / 100;
    }
    if (owns(GAUNTLET_BOON_MOMENTUM) && Boon.streak != 0)
    {
        damage = damage * (100 + 5 * Boon.streak) / 100;
    }
    if (owns(GAUNTLET_BOON_RIPOSTE) && Boon.riposte)
    {
        damage *= 2;
    }
    if (owns(GAUNTLET_BOON_LIGHT_OF_EARENDIL) && victim != NULL &&
        PLAYER(playerIndex).ownerActor != NULL)
    {
        Actor *owner = PLAYER(playerIndex).ownerActor;
        s32 dx = (victim->coords.x - owner->coords.x) >> 16;
        s32 dy = (victim->coords.y - owner->coords.y) >> 16;

        if (ABS(dx) + ABS(dy) <= LIGHT_RADIUS)
        {
            damage = damage * 130 / 100;
        }
    }
    return damage;
}

s32 gauntlet_damageDealt(u8 playerIndex, Actor *victim, s32 damage)
{
    const HackLibDamageSource *source = hackLib_currentDamageSource();
    if (!gauntlet_isRunning() || (source != NULL && source->kind == HACKLIB_DAMAGE_SECONDARY))
    {
        return damage;
    }
    if (owns(GAUNTLET_BOON_EDGE_OF_ANDURIL))
    {
        damage = damage * 115 / 100;
    }
    if (owns(GAUNTLET_BOON_BLOOD_PRICE) && PLAYER(playerIndex).currentHp >= maxHp(playerIndex))
    {
        damage = damage * 125 / 100;
    }
    if (owns(GAUNTLET_BOON_EXECUTIONER) && victim != NULL &&
        victim->currentHp * 4 < foeMaxHp(victim))
    {
        damage *= 3;
    }
    if (Boon.lastStandTicks != 0)
    {
        damage *= 3;
    }
    return gauntletBoonEffects_damage(playerIndex, victim,
                                      scaleDamage(playerIndex, victim, damage));
}

static void chainLightning(Actor *owner, const HackLibCombatEvent *event)
{
    Actor *foe = NULL;
    u32 arcs = 0;

    if (gauntlet_randBelow(100) >= 15)
    {
        return;
    }
    while (arcs < 2 && (foe = hackLib_nextTargetAt(&event->victim->coords, event->victim,
                                                   CHAIN_RADIUS, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe))
        {
            continue;
        }
        hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_BLUE, FX_LAYER_FRONT);
        if (owns(GAUNTLET_BOON_STORM_PYRE))
        {
            gauntletBoons_ignite(foe);
        }
        boonDamage(owner, foe, event->result.damageDealt / 2);
        arcs++;
    }
}

static void corpseBlast(Actor *owner, const HackLibCombatEvent *event)
{
    Actor *foe = NULL;
    s32 damage = foeMaxHp(event->victim) / 2;

    hackLib_fxAt(owner, &event->victim->coords, FX_SHAPE_POOL_FLASH, FX_COLOR_EMBER);
    while ((foe = hackLib_nextTargetAt(&event->victim->coords, event->victim, CORPSE_RADIUS,
                                       foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            boonDamage(owner, foe, damage);
        }
    }
}

static void hasteLoop(u8 playerIndex)
{
    u16 *cooldowns = skillExpansion_runtime[playerIndex].cooldowns;
    u32 row;

    for (row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        cooldowns[row] =
            cooldowns[row] > HASTE_LOOP_FRAMES ? cooldowns[row] - HASTE_LOOP_FRAMES : 0;
    }
}

/** Boon damage to every foe within radius of a point. */
static void areaBurst(Actor *owner, const Vector2Fp16 *center, Actor *exclude, u16 radius,
                      s32 damage, u8 shape, HackLibFxColor color)
{
    Actor *foe = NULL;

    hackLib_fxAt(owner, center, shape, color);
    while ((foe = hackLib_nextTargetAt(center, exclude, radius, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            boonDamage(owner, foe, damage);
        }
    }
}

static void stunAround(Actor *owner, u16 radius, s32 frames)
{
    Actor *foe = NULL;

    while ((foe = hackLib_nextTargetAt(&owner->coords, owner, radius, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            hackLib_applyStun(foe, frames);
        }
    }
    hackLib_fxBurst(owner, FX_SHAPE_WIDE_RING, FX_COLOR_WHITE, FX_LAYER_BEHIND);
}

/** The hit triggers of the newer boons. */
static void onHit(Actor *owner, const HackLibCombatEvent *event)
{
    Boon.riposte = FALSE;
    if (event->result.damageDealt <= 0)
    {
        return;
    }
    if (owns(GAUNTLET_BOON_STATIC_CHARGE) && ++Boon.hits >= 8)
    {
        Boon.hits = 0;
        areaBurst(owner, &owner->coords, owner, 56, event->result.damageDealt * 2,
                  FX_SHAPE_TENDRIL_STAR, FX_COLOR_BLUE);
    }
    if (owns(GAUNTLET_BOON_AVALANCHE) &&
        gauntlet_randBelow(100) < (gauntlet_roomsCleared() < 20 ? 5 + gauntlet_roomsCleared() : 25))
    {
        Actor *center = event->victim;
        s32 amount = event->result.damageDealt * 3 / 2;
        if (owns(GAUNTLET_BOON_SIEGE_ENGINE))
        {
            for (Actor *cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
            {
                if (gauntlet_isLivingRunFoe(cursor) &&
                    (center->currentHp <= 0 || cursor->currentHp > center->currentHp))
                {
                    center = cursor;
                }
            }
            amount *= 2;
        }
        areaBurst(owner, &center->coords, NULL, 40, amount, FX_SHAPE_SMOKE_RISE, FX_COLOR_EMBER);
    }
}

/** The kill triggers of the newer boons. */
static void onKill(u8 playerIndex, Actor *owner)
{
    if (owns(GAUNTLET_BOON_SIPHON_MIND))
    {
        s32 cap = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
        if (owns(GAUNTLET_BOON_OVERFLOW))
        {
            cap = cap * 3 / 2;
        }
        s32 spirit = PLAYER(playerIndex).currentSpirit +
                     PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT) * 5 / 100 + 1;

        PLAYER(playerIndex).currentSpirit = spirit > cap ? cap : spirit;
    }
    if (owns(GAUNTLET_BOON_BATTLE_CRY) && ++Boon.kills >= 20)
    {
        Boon.kills = 0;
        stunAround(owner, 80, 2 * TICKS_PER_SECOND);
        skillExpansion_feedback(playerIndex, "Battle Cry!");
    }
    if (owns(GAUNTLET_BOON_MOMENTUM))
    {
        Boon.streak = Boon.streak < 5 ? Boon.streak + 1 : 5;
        Boon.streakTicks = 3 * TICKS_PER_SECOND;
    }
}

void gauntlet_combatEvent(u8 playerIndex, const HackLibCombatEvent *event)
{
    Actor *owner = PLAYER(playerIndex).ownerActor;

    if (gauntlet_isRunning() && event->victim != NULL && event->result.killed)
    {
        gauntlet_noteKill();
    }
    if (!gauntlet_isRunning() || owner == NULL || event->victim == NULL || !event->result.hit)
    {
        return;
    }
    gauntletBoonEffects_event(playerIndex, event);
    bool primary = event->source.kind != HACKLIB_DAMAGE_SECONDARY &&
                   event->source.kind != HACKLIB_DAMAGE_PERIODIC;
    if (primary && owns(GAUNTLET_BOON_CHAIN_LIGHTNING) && event->result.damageDealt > 0)
    {
        chainLightning(owner, event);
    }
    if (primary)
    {
        onHit(owner, event);
    }
    if (!event->result.killed)
    {
        return;
    }
    if (gauntletBoons_has(GAUNTLET_BOON_VETERANS_PURSE))
    {
        if (Boon.veteranKills < 30 && ++Boon.veteranKills == 30)
        {
            skillExpansion_feedback(playerIndex, "Veteran's Purse evolved!");
        }
        u32 gems = PLAYER(playerIndex).gems + (Boon.veteranKills >= 30 ? 3 : 1);
        PLAYER(playerIndex).gems = gems > 65535 ? 65535 : gems;
    }
    onKill(playerIndex, owner);
    if (owns(GAUNTLET_BOON_BLOODTHIRST))
    {
        skillExpansion_addHp(playerIndex, maxHp(playerIndex) * 2 / 100 + 1, FALSE);
    }
    if (owns(GAUNTLET_BOON_HASTE_LOOP))
    {
        hasteLoop(playerIndex);
    }
    if (event->source.kind != HACKLIB_DAMAGE_SECONDARY && owns(GAUNTLET_BOON_CORPSE_BLAST))
    {
        corpseBlast(owner, event);
    }
}

void gauntlet_enemyHitResolved(u8 playerIndex, s32 hpBefore)
{
    Actor *owner = PLAYER(playerIndex).ownerActor;

    if (!gauntlet_isRunning() || PLAYER(playerIndex).currentHp >= hpBefore)
    {
        return;
    }
    if (owns(GAUNTLET_BOON_HOBBIT_LUCK) && gauntlet_randBelow(10) == 0)
    {
        PLAYER(playerIndex).currentHp = hpBefore;
        Boon.riposte = TRUE;
        return;
    }
    {
        s32 lost = hpBefore - PLAYER(playerIndex).currentHp;
        const HackLibDamageSource *source = hackLib_currentDamageSource();

        Boon.riposte = TRUE;
        if (owns(GAUNTLET_BOON_IRON_SKIN))
        {
            PLAYER(playerIndex).currentHp += lost / 10;
        }
        if (owns(GAUNTLET_BOON_GLASS_CANNON))
        {
            PLAYER(playerIndex).currentHp -= lost / 4;
        }
        if (owns(GAUNTLET_BOON_THORNS) && owner != NULL && source != NULL &&
            source->origin != NULL && skillExpansion_isFoe(source->origin))
        {
            hackLib_fxBurst(source->origin, FX_SHAPE_SPARK, FX_COLOR_EMBER, FX_LAYER_FRONT);
            boonDamage(owner, source->origin, lost * 30 / 100);
        }
    }
    if (owns(GAUNTLET_BOON_LAST_STAND) && PLAYER(playerIndex).currentHp <= 0 &&
        !(Boon.spent & SPENT_LAST_STAND) && owner != NULL)
    {
        Boon.spent |= SPENT_LAST_STAND;
        PLAYER(playerIndex).currentHp = 1;
        Boon.lastStandTicks = LAST_STAND_TICKS;
        owner->flags.p |= ACTOR_FLAG_INVULNERABLE;
        hackLib_fxBurst(owner, FX_SHAPE_TENDRIL_STAR, FX_COLOR_GOLD, FX_LAYER_FRONT);
        skillExpansion_feedback(playerIndex, "Last Stand!");
        return;
    }
    if (owns(GAUNTLET_BOON_SECOND_LIFE) && PLAYER(playerIndex).currentHp <= 0 &&
        !(Boon.runSpent & 1) && owner != NULL)
    {
        Boon.runSpent |= 1;
        PLAYER(playerIndex).currentHp = maxHp(playerIndex) / 2;
        hackLib_fxBurst(owner, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
        skillExpansion_feedback(playerIndex, "Second Life!");
    }
}
