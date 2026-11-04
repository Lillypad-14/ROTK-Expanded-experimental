#pragma once

// Skill kits written as data (PC edition). hacks/tools/hero_kit.py compiles
// hacks/content/heroes/<hero>.json into kit tables run by this engine. See
// docs/wip-docs/howto-heroes-skills.md, "Skill specs".

#include "patches/skill_expansion/skill_delivery.h"
#include "patches/skill_expansion/skill_expansion.h"

typedef enum SpecEffect
{
    // Launch a delivery: a projectile, or a burst at the caster or the nearest foe.
    SPEC_EFFECT_DELIVERY = 0,
    // Heal the caster by amount HP.
    SPEC_EFFECT_HEAL = 1,
    // The caster takes amount percent less damage for frames.
    SPEC_EFFECT_WARD = 2,
    // The caster's melee hits deal amount percent more for frames.
    SPEC_EFFECT_EMPOWER = 3,
    // count allies fight beside the caster for frames, each hit dealing amount.
    SPEC_EFFECT_SUMMON = 4,
    // Kill the nearest foe within targetRange at threshold percent HP or less, heal amount HP and
    // count killCredit kills toward the next rebirth charge.
    SPEC_EFFECT_DEVOUR = 5,
    // Pay amount percent of max HP for one rebirth charge.
    SPEC_EFFECT_TITHE = 6,
    // A rider (chargeAnim) charges along the caster's facing for frames, dealing amount and
    // stunFrames to each foe it runs over.
    SPEC_EFFECT_CHARGE = 7,
    // A flyer (chargeAnim) dives from the sky onto the nearest foe within targetRange over frames,
    // then plays strikeAnim and launches the delivery there. Target fx layers play on arrival.
    SPEC_EFFECT_SWOOP = 8,
    // Foes within chargeRadius are stunned and pushed away from the caster at chargeSpeed px per
    // tick for frames (they flee), taking amount once.
    SPEC_EFFECT_FEAR = 9,
    // The caster turns intangible for frames: takes no damage, and every foe within chargeRadius
    // takes amount (and status) every few ticks.
    SPEC_EFFECT_PHASE = 10,
    // A zone stays where it was cast for frames: every trailTicks it plays trail and hits foes
    // within chargeRadius for amount (and status).
    SPEC_EFFECT_ZONE = 11,
    // A zone that follows the caster, healing percent of the damage it deals.
    SPEC_EFFECT_DRAIN = 12,
    // Foes within chargeRadius are held in place for frames (they can still attack), taking amount.
    SPEC_EFFECT_ROOT = 13,
    // count riders (chargeAnim) charge side by side from behind the caster along its facing.
    SPEC_EFFECT_STAMPEDE = 14,
    // The nearest foe within targetRange fights for the caster for frames: it stops attacking him,
    // walks to the nearest other foe within chargeRadius and strikes it for amount every second.
    SPEC_EFFECT_CHARM = 15,
    // Foes within chargeRadius take percent more damage from the players for frames.
    SPEC_EFFECT_MARK = 16,
    // Foes within chargeRadius take amount (and status), and the caster gains percent Spirit each.
    SPEC_EFFECT_DISMAY = 17,
    // The caster himself rushes along his facing at chargeSpeed for frames, untouchable, hitting each
    // foe within chargeRadius once for amount and stunFrames.
    SPEC_EFFECT_DASH = 18,
    // For frames every kill heals the caster amount HP and gives percent Spirit.
    SPEC_EFFECT_BOUNTY = 19,
    // Up to count foes within targetRange are marked, and frames later each takes amount (and status).
    SPEC_EFFECT_BARRAGE = 20,
    // The caster pays percent of his max HP, then for frames his spell and skill damage (ranged
    // hits, not melee) is amount percent higher.
    SPEC_EFFECT_DESPAIR = 21,
    // The caster leaps onto the nearest foe within targetRange at chargeSpeed, untouchable, biting
    // it for amount and stunFrames, then on to count more foes (a chain).
    SPEC_EFFECT_LEAP = 22,
    // Foes within chargeRadius deal percent less damage to the players for frames.
    SPEC_EFFECT_WEAKEN = 23,
    // For frames each melee hit adds a bleed stack (lasting statusFrames): amount damage per stack
    // every second, sparks in chargeColor.
    SPEC_EFFECT_BLEED = 24,
    // BLEED's stacking for venom: at count stacks the foe is paralyzed for stunFrames.
    SPEC_EFFECT_VENOM = 25,
    // The most wounded foe within targetRange takes amount, three times over below percent HP.
    SPEC_EFFECT_FINISHER = 26,
    // Adds amount Frenzy stacks and raises the Frenzy cap to count for frames.
    SPEC_EFFECT_FRENZY = 27,
    // count traps (1 at the caster, more in a ring spread pixels out) wait frames, showing trail.
    // The first foes within chargeRadius are held for stunFrames and take amount (and status).
    SPEC_EFFECT_TRAP = 28,
    // The caster scuttles backwards at chargeSpeed, untouchable, leaving a trap where he stood.
    SPEC_EFFECT_BACKSTEP = 29,
    // The nearest foe within targetRange is dragged to the caster at chargeSpeed, stunned.
    SPEC_EFFECT_PULL = 30,
    // The nearest foe within targetRange sleeps for frames (wrapped in trail) until it is hit.
    SPEC_EFFECT_SLEEP = 31,
    // A whip line along the caster's facing, targetRange long and chargeRadius to each side: every
    // foe on it takes amount and stunFrames. trail plays along the line.
    SPEC_EFFECT_LASH = 32,
    // Foes in a cone ahead of the caster within chargeRadius take amount, are kicked away at
    // chargeSpeed and lie stunned for stunFrames.
    SPEC_EFFECT_SHOVE = 33,
    // Foes within chargeRadius flinch for stunFrames, and the caster takes amount percent less damage
    // per foe (at most percent) for frames.
    SPEC_EFFECT_TAUNT = 34,
    // For frames the caster walks amount percent faster and gains percent Spirit every second.
    SPEC_EFFECT_HASTE = 35,
    // Heal amount HP, plus amount for each foe within chargeRadius (up to 5).
    SPEC_EFFECT_SECOND_WIND = 36,
    // For frames each melee hit also strikes every other foe within chargeRadius for amount percent of it.
    SPEC_EFFECT_CLEAVE = 37,
    // For frames a foe that strikes the caster takes amount percent of the blow back (trail on it).
    SPEC_EFFECT_REFLECT = 38,
    // For frames, every trailTicks, foes within chargeRadius of the caster cower for stunFrames.
    SPEC_EFFECT_DREAD = 39,
    // For frames no blow takes the caster below 1 HP. When it ends he heals amount.
    SPEC_EFFECT_UNDYING = 40,
    // The next amount damage the caster takes within frames is soaked up.
    SPEC_EFFECT_ABSORB = 41,
    // For frames the caster's melee hits heal him amount percent of their damage.
    SPEC_EFFECT_LIFESTEAL = 42,
    // For frames blows from foes farther than chargeRadius lose amount percent.
    SPEC_EFFECT_RAMPART = 43,
    // For frames the caster's melee hits deal amount percent more and he takes percent more damage.
    SPEC_EFFECT_MADNESS = 44,
    // The caster hooks the nearest foe within targetRange (trail drawn along the line) and is hauled
    // to it at chargeSpeed, untouchable, striking it for amount and stunFrames on arrival.
    SPEC_EFFECT_GRAPPLE = 45,
    // The nearest foe within targetRange takes count cuts of amount, one every trailTicks (trail on
    // each), held in place while they land.
    SPEC_EFFECT_FLURRY = 46,
    // Foes within chargeRadius take amount, and for frames their blows on the players miss percent of
    // the time.
    SPEC_EFFECT_BLIND = 47,
    // For frames each foe the caster's player kills drops loot amount percent of the time (trail
    // there).
    SPEC_EFFECT_PLUNDER = 48,
    // A keg (trail, held) at the caster's feet blows after frames: foes within chargeRadius take
    // amount and stunFrames, target layers play at the blast.
    SPEC_EFFECT_BOMB = 49,
    // count blasts walk out along the caster's facing, spread pixels apart, one every trailTicks:
    // foes within chargeRadius of each take amount and stunFrames, target layers play at each.
    SPEC_EFFECT_BROADSIDE = 50,
    // A bolt flies targetRange along the caster's facing (trail each tick): every foe within
    // chargeRadius of its line takes amount and stunFrames.
    SPEC_EFFECT_PIERCE = 51,
    // count bolts fan out around the caster's facing, each a pierce.
    SPEC_EFFECT_FAN = 52,
    // A shield (trail, held) planted ahead for frames: while the caster stands within chargeRadius
    // of it he takes amount percent less damage.
    SPEC_EFFECT_COVER = 53,
    // A burning bolt (trail in flight) strikes the nearest foe within targetRange for amount (and
    // status). If it dies within frames, the fire leaps to every foe within chargeRadius of it.
    SPEC_EFFECT_IGNITE = 54,
    // For frames each second the caster stands still adds amount percent to his damage, at most
    // percent (trail each second he holds).
    SPEC_EFFECT_AIM = 55,
    // For frames, every trailTicks, a bolt drops on a random spot within chargeRadius of where he
    // stood: foes near it take amount and stunFrames, target layers play there.
    SPEC_EFFECT_HAIL = 56,
} SpecEffect;

#define SPEC_FX_COUNT 3
#define SPEC_LOOK_NONE 0xff
// SpecFx.flags: play on the skill's target instead of the caster.
#define SPEC_FX_AT_TARGET 1

// One layer of a skill's look: an fx_kit shape, a sprite animation, or a skill_fx.json effect.
typedef struct SpecFx
{
    // A sprite animation (one facing's record) played instead of shape, NULL for a shape.
    const SpriteAnimation *anim;
    // HackLibFxColor, 0 for no layer unless effect is set.
    u16 color;
    // A shape: 0 plays it once, otherwise it loops this long. An anim or effect: frames it holds
    // its last frame, 0 for none.
    u16 frames;
    // HackLibFxShape.
    u8 shape;
    // HackLibFxLayer.
    u8 layer;
    // SPEC_FX_* bits.
    u8 flags;
    // An anim's ticks per frame.
    u8 ticks;
    // A skill_fx.json effect id + 1 played instead of shape or anim, 0 for none.
    u8 effect;
    // Frames after the cast the layer waits, at the spot it would have played, 0 for at once.
    u8 delay;
} SpecFx;

typedef struct SpecSkill
{
    // SPEC_EFFECT_DELIVERY only.
    const SkillDeliveryDef *delivery;
    // Per skill level 1..5: HP healed, the ward or empower percent, an ally's hit damage, or the
    // tithe's percent of max HP.
    u16 amount[5];
    // Ward, empower or ally duration.
    u16 frames;
    // Cast sound (sfx id), 0 for none.
    u16 sound;
    // SpecEffect.
    u8 effect;
    // Nonzero: delivered at, or devouring, the nearest foe within this many pixels.
    u8 targetRange;
    // Allies per summon.
    u8 count;
    // Devour: highest HP percent a foe may have.
    u8 threshold;
    // Devour: kills counted toward the next rebirth charge.
    u8 killCredit;
    // Summon: the enemy type the allies look like, SPEC_LOOK_NONE for the stock ghost.
    u8 lookEnemy;
    // Summon: the allies' HackLibFxColor, 0 for the look enemy's own palette.
    u16 lookColor;
    SpecFx fx[SPEC_FX_COUNT];
    // Heal, ward or empower: a burst around the caster as well (knockback, stun), or NULL.
    const SkillDeliveryDef *burst;
    // Frames the caster's summoned allies stay longer, 0 for none.
    u16 rallyFrames;
    // Empower: a SkillStatusKind each melee hit applies while it lasts, and its frames and power.
    u8 status;
    u8 pad2;
    u16 statusFrames;
    u16 statusPower;
    // Charge, swoop and stampede: the rider's animation (facing record 0), color, speed in pixels
    // per tick, reach and the stun each foe takes. Fear, phase, zone, drain and root use
    // chargeRadius as their reach (and fear chargeSpeed as the flight speed).
    const SpriteAnimation *chargeAnim;
    u16 chargeColor;
    u8 chargeSpeed;
    u8 chargeRadius;
    u16 stunFrames;
    // Swoop: the animation (facing record 0) the flyer plays on arrival, NULL to vanish.
    const SpriteAnimation *strikeAnim;
    // Charge or swoop: a layer left behind the mover every trailTicks, none when trail.color and
    // trail.effect are 0.
    SpecFx trail;
    u8 trailTicks;
    // Summon: pixels between the caster and each ally, 0 for the default.
    u8 spread;
    // Drain: percent of the damage dealt that heals the caster. Mark: extra damage percent.
    // Dismay: Spirit per foe. Bounty: Spirit per kill. Taunt: the most damage taken off. Haste:
    // Spirit per second.
    u8 percent;
} SpecSkill;

// A killing blow on a hero with charges leaves 1 HP, keeps the hero untouchable for frames, then
// heals and bursts around the hero. Kills earn charges back.
typedef struct SpecRebirth
{
    // Untouchable time after the killing blow.
    u16 frames;
    // Damage of the burst on rising.
    u16 burstDamage;
    // Sound on rising (sfx id), 0 for none.
    u16 sound;
    u8 maxCharges;
    // Charges at the start of every zone, unless the hero holds more.
    u8 startCharges;
    u8 killsPerCharge;
    // Max HP percent restored on rising.
    u8 healPercent;
    // Burst radius in pixels.
    u8 burstRadius;
    // Burst stun in frames.
    u8 burstStun;
} SpecRebirth;

// Frenzy: each melee hit adds a stack (up to maxStacks), each stack adds perStack percent melee
// damage, and the stacks drain once no hit lands for decayFrames. The HUD shows them.
typedef struct SpecFrenzy
{
    u16 decayFrames;
    u8 maxStacks;
    u8 perStack;
} SpecFrenzy;

// A primary attack that looses a missile: each melee swing launches delivery from the hero along his
// facing when it reaches frame, at level 1 plus one per 5 hero levels (at most 5).
typedef struct SpecPrimary
{
    const SkillDeliveryDef *delivery;
    u8 frame;
} SpecPrimary;

// Where a hero's melee bank sits in the hero data region (hacks/tools/hero_data.py).
#define HERO_DATA_BODIES 0x8000
#define HERO_DATA_BODY_BYTES 0xa00
#define HERO_DATA_BANK_BYTES 0xe0
#define HERO_MELEE_BANK(heroId) \
    ((const void *)(MEM_HERO_DATA + HERO_DATA_BODIES + (heroId) * HERO_DATA_BODY_BYTES + HERO_DATA_BANK_BYTES))

// What a spec passive does besides its stat records.
typedef enum SpecPassiveEffect
{
    SPEC_PASSIVE_STATS = 0,
    // amount more rebirth charges at most.
    SPEC_PASSIVE_REBIRTH_CHARGES = 1,
    // Rebirth heals amount percent of max HP more.
    SPEC_PASSIVE_REBIRTH_HEAL = 2,
    // A rebirth charge takes amount fewer kills.
    SPEC_PASSIVE_CHARGE_KILLS = 3,
    // Summoned allies stay amount seconds longer.
    SPEC_PASSIVE_SUMMON_SECONDS = 4,
    // Devour takes foes with amount percent more HP left.
    SPEC_PASSIVE_DEVOUR_THRESHOLD = 5,
    // Summons bring amount more allies.
    SPEC_PASSIVE_SUMMON_COUNT = 6,
} SpecPassiveEffect;

// A passive written in a spec: an elite passive (name, icon, stats) plus a spec engine effect.
typedef struct SpecPassive
{
    SkillExpansionElitePassive def;
    // SpecPassiveEffect.
    u8 effect;
    // Effect size per purchased level 1..5.
    u8 amount[5];
} SpecPassive;

// What a kit slot's specRows points at. rows comes first, so the slot still reads as rows.
typedef struct SpecKitTable
{
    // Page 1 rows 0..5, page 2 rows 7..12. Row 6 is unused.
    const SpecSkill *rows[13];
    // NULL: the hero dies like any other.
    const SpecRebirth *rebirth;
    // The hero's spec passives by id offset (SKILL_EXPANSION_SPEC_PASSIVE_FIRST), NULL where unused.
    const SpecPassive *passives[HERO_PASSIVE_SKILL_COUNT];
    // NULL: no Frenzy.
    const SpecFrenzy *frenzy;
    // NULL: the melee swings strike as drawn.
    const SpecPrimary *primary;
} SpecKitTable;

#define SPEC_KIT_CAST_ROW(cost)                                                                    \
    {.spiritCost = cost,                                                                           \
     .field_0x20 = 1,                                                                              \
     .castFlags = {.d = {.forcePose0 = 1}},                                                        \
     .field_0x22 = 5,                                                                              \
     .castTriggerFrame = 0,                                                                        \
     .field_0x24 = 0xff}

extern const SkillExpansionCastPolicy SpecKitQuickPolicy;
extern const SkillExpansionCastPolicy SpecKitNormalPolicy;
extern const SkillExpansionCastPolicy SpecKitLongPolicy;
extern const SkillExpansionCastPolicy SpecKitUltimatePolicy;

SkillExpansionCastResult specKit_resolve(Actor *actor);
void specKit_reset(u8 playerIndex);
void specKit_cleanup(u8 playerIndex, Actor *actor);
void specKit_tick(u8 playerIndex, Actor *actor);
s32 specKit_meleeDamage(u8 playerIndex, Actor *victim, s32 damage);
s32 specKit_rangedDamage(u8 playerIndex, Actor *victim, s32 damage);
void specKit_onKill(u8 playerIndex, Actor *attacker, Actor *victim);
s32 specKit_incomingDamage(u8 playerIndex, Actor *attacker, s32 damage);
void specKit_drawHud(u8 playerIndex);
const SkillExpansionElitePassive *specKit_passive(u32 passiveSkillId);
