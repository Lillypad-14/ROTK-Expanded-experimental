#if HACK_gauntlet
#include "patches/gauntlet/gauntlet_boons.h"
#endif
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "actor/npc.h"
#include "combat.h"
#include "enemy.h"
#include "fx.h"
#include "game.h"
#include "lib/actor_ref.h"
#include "lib/combat_result.h"
#include "lib/fx_kit.h"
#include "lib/hud_meter.h"
#include "lib/profile.h"
#include "lib/status.h"
#include "lib/targeting.h"
#include "memory.h"
#include "mission.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "patches/skill_expansion/skill_expansion_eowyn.h"
#include "patches/skill_expansion/skill_expansion_frodo.h"
#include "patches/skill_expansion/skill_expansion_gandalf.h"
#include "patches/skill_expansion/skill_expansion_gimli.h"
#include "patches/skill_expansion/skill_expansion_legolas.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "patches/skill_expansion/skill_expansion_sam.h"
#include "patches/skill_expansion/skill_expansion_smeagol.h"
#include "rng.h"
#include "sfx.h"
#include "stats.h"
#include "variables.h"

#if HACK_twists
#include "patches/twists/twists.h"
#endif
#if HACK_gauntlet
#include "patches/gauntlet/gauntlet.h"
#include "patches/gauntlet/gauntlet_boon_effects.h"
#include "patches/gauntlet/gauntlet_boons.h"
#endif
#if HACK_debug_cheats
#include "patches/debug_cheats/debug_cheats.h"
#include "patches/debug_cheats/debug_lab.h"
#endif
#if HACK_ringwraith
#include "patches/ringwraith/ringwraith_kit.h"
#endif

// Game logic and actor ticks run at 30 frames per second.
#define FRAMES_PER_SECOND 30

// Anduril's Flame meter
#define METER_MAX 100
#define METER_HIT 4
#define METER_KILL 6
#define METER_STUN 4
#define METER_AMBUSH 20
#define METER_RIPOSTE 8
#define METER_OATH_GHOST 10
#define METER_MARK_KILL 25
#define METER_HURT 15
#define METER_IDLE_FRAMES (4 * FRAMES_PER_SECOND)
#define METER_DECAY_FRAMES (FRAMES_PER_SECOND / 2)
#define REFORGED_PERCENT 10
#define BAR_FLASH_FRAMES 8
#define BAR_PULSE_FRAMES 16
// WeaponPrefixes rows borrowed for their blade glows: Fireburst for a full bar, Sunburst for the
// blaze.
#define CHARGED_GLOW_PREFIX 150
#define BLAZE_GLOW_PREFIX 55
// Fire damage each melee blow adds while the Flame bar is full.
#define CHARGED_FIRE_DAMAGE 4
#define WEAPON_HAND 0
#define WEAPON_INVENTORY_SLOT 1

// Flame of the West
#define FLAME_BASE_FRAMES (4 * FRAMES_PER_SECOND)
#define FLAME_FRAMES_PER_LEVEL FRAMES_PER_SECOND
#define FLAME_FULL_FRAMES (8 * FRAMES_PER_SECOND)
#define FLAME_KILL_FRAMES FRAMES_PER_SECOND
#define FLAME_BURST_BASE_DAMAGE 3
#define FLAME_BURST_DAMAGE_PER_LEVEL 3
#define FLAME_RING_BASE_DAMAGE 10
#define FLAME_RING_DAMAGE_PER_LEVEL 5

// Ranger's Ambush
#define AMBUSH_BASE_FRAMES (2 * FRAMES_PER_SECOND)
#define AMBUSH_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 2)
#define AMBUSH_CRIT_BASE_PERCENT 150
#define AMBUSH_CRIT_PERCENT_PER_LEVEL 50
#define AMBUSH_STUN_FRAMES (3 * FRAMES_PER_SECOND / 2)
#define AMBUSH_SFX 0x47

// Actor.spriteFlags bit of a stealthed (see-through) player.
#define SPRITE_FLAG_STEALTH 0x20

// Tracker's Mark
#define MARK_RADIUS 160
#define MARK_BASE_FRAMES (10 * FRAMES_PER_SECOND)
#define MARK_FRAMES_PER_LEVEL (2 * FRAMES_PER_SECOND)
#define MARK_BASE_PERCENT 15
#define MARK_PERCENT_PER_LEVEL 5
#define MARK_FX_FRAMES FRAMES_PER_SECOND

// Oath of the Dead
#define OATH_RADIUS 48
#define OATH_BASE_DAMAGE 15
#define OATH_DAMAGE_PER_LEVEL 8
#define OATH_STUN_FRAMES FRAMES_PER_SECOND

// Return of the King
#define RETURN_BASE_FRAMES (10 * FRAMES_PER_SECOND)
#define RETURN_FRAMES_PER_LEVEL (2 * FRAMES_PER_SECOND)
#define RETURN_TEMPORARY_LEVELS 1

// Passives
#define RIPOSTE_PERCENT_PER_LEVEL 10
#define RIPOSTE_RADIUS 40
#define PATIENCE_READY_FRAMES (3 * FRAMES_PER_SECOND / 2)
#define PATIENCE_PERCENT_PER_LEVEL 20
#define METER_PATIENCE 10
#define QUARRY_FRAMES_PER_LEVEL (2 * FRAMES_PER_SECOND)
#define PRESENCE_STUN_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 2)
#define PRESENCE_DAMAGE_PER_LEVEL 2
#define RESTLESS_EXPIRY_FRAMES 3 // a ghost bursts once its lifetime falls to this
#define RESTLESS_BASE_DAMAGE 6
#define RESTLESS_DAMAGE_PER_LEVEL 6
#define RESTLESS_STUN_FRAMES FRAMES_PER_SECOND
#define METER_RESTLESS 5
#define CHAIN_WINDOW_FRAMES (3 * FRAMES_PER_SECOND)
#define DEAD_RIDE_BASE_KILLS 8 // chained kills per ghost are this minus the level
#define DEAD_RIDE_MAX_GHOSTS 4
#define BLADE_HP_PERCENT 30
#define BLADE_BASE_COOLDOWN_FRAMES (90 * FRAMES_PER_SECOND)
#define BLADE_COOLDOWN_FRAMES_PER_LEVEL (10 * FRAMES_PER_SECOND)
#define BLADE_FLAME_FRAMES (4 * FRAMES_PER_SECOND)
#define HANDS_PERCENT_PER_LEVEL 2
#define LORD_PERCENT 10
#define LORD_MAX_GHOSTS 3

// Page 1 skills
#define METER_SWEEP 8
#define METER_KINGS_COMMAND 10
#define METER_GHOST_HIT 2
#define METER_HERB 15
#define WILL_FRAMES_PER_LEVEL (5 * FRAMES_PER_SECOND)

typedef struct EliteState
{
    // Flame of the West frames left
    u16 flameFrames;

    // Ranger's Ambush stealth frames left
    u16 ambushFrames;

    // Tracker's Mark frames left
    u16 markFrames;

    // frames since Aragorn last stoked Anduril's Flame
    u16 meterIdleFrames;

    // the marked foe while markFrames is running
    HackLibActorRef markTarget;

    // Return of the King frames left
    u16 returnFrames;

    // Numenorean Will frames left
    u16 willFrames;

    // frames until The Blade Reforged can fire again
    u16 bladeCooldown;

    // frames left to extend the kill chain
    u16 chainFrames;

    // currentHp at the previous tick
    s16 lastHp;

    // row spirit costs saved by Return of the King
    u16 savedSpiritCosts[SKILL_EXPANSION_ACTIVE_ROW_COUNT];

    // cast level of each timed active
    u8 flameLevel;
    u8 ambushLevel;
    u8 markLevel;

    // Anduril's Flame, 0..METER_MAX
    u8 meter;

    // TRUE while the current Flame of the West was cast at a full meter
    u8 flameReforged;

    // meter value at the last bar draw
    u8 barMeter;

    // frames left of the bar's change flash
    u8 barFlashFrames;

    // full bar pulse phase, 0..2 * BAR_PULSE_FRAMES - 1
    u8 barPulseFrames;

    // kills in the current chain
    u8 chainKills;

    // TRUE while the ambush strike's melee attack resolves
    u8 ambushStrike;

    // WeaponPrefixes row of the blade glow the Flame lit, 0 when none
    u8 chargedGlow;

    // frames since the last melee blow, capped at PATIENCE_READY_FRAMES
    u8 patienceFrames;

    // TRUE while a Hunter's Patience blow resolves
    u8 patienceStrike;
} EliteState;

static EliteState states[2];
u32 skillExpansion_heroState[2][SKILL_EXPANSION_HERO_STATE_WORDS];
u8 skillExpansion_heroStateOwner[2];
u8 skillExpansion_heroStateMisuse;

__attribute__((noinline)) void skillExpansion_noteHeroStateMisuse(void)
{
    if (skillExpansion_heroStateMisuse != 0xff)
    {
        skillExpansion_heroStateMisuse++;
    }
}

asm(".section .text\n"
    ".balign 4\n"
    ".thumb\n"
    ".global skillExpansion_original_player_addHp\n"
    ".thumb_func\n"
    "skillExpansion_original_player_addHp:\n"
    "    push {r4-r7, lr}\n"
    "    lsl r0, r0, #24\n"
    "    lsr r0, r0, #24\n"
    "    ldr r3, =player_addHp + 8\n"
    "    mov ip, r3\n"
    "    add r3, r0, #0\n"
    "    bx ip\n"
    "    .pool\n");

typedef struct IncomingResolution
{
    const HackLibDamageSource *source;
    u32 damage;
    u8 playerIndex;
    bool observed;
    bool killed;
    Actor *victim;
    u8 appliedStatusElements;
} IncomingResolution;

typedef struct HpChangeObservation
{
    struct HpChangeObservation *previous;
    s32 childDelta;
    u8 playerIndex;
} HpChangeObservation;

static IncomingResolution *currentIncomingResolution;
static HpChangeObservation *currentHpObservation;

void skillExpansion_addHp(u8 playerIndex, s16 amount, bool flinchOnDamage)
{
    const SkillExpansionHeroKit *kit;

    if (playerIndex >= 2)
    {
        return;
    }
#if HACK_gauntlet
    s32 adjusted = amount;
    gauntletBoonEffects_hp(playerIndex, &adjusted, &flinchOnDamage);
    amount = adjusted < -32767 ? -32767 : adjusted > 32767 ? 32767 : adjusted;
    if (amount == 0)
    {
        flinchOnDamage = FALSE;
    }
#endif
    IncomingResolution *incoming = currentIncomingResolution;
    if (amount >= 0 || incoming == NULL || incoming->playerIndex != playerIndex ||
        incoming->source != hackLib_currentDamageSource())
    {
        incoming = NULL;
    }
    if (incoming != NULL)
    {
        incoming->observed = TRUE;
    }
    if (amount < 0)
    {
        const HackLibDamageSource *source = hackLib_currentDamageSource();
        s32 damage = skillExpansion_frodoIncomingDamage(
            playerIndex, source != NULL ? source->origin : NULL, -(s32)amount);

        damage = skillExpansion_smeagolIncomingDamage(playerIndex, damage);
        damage = skillExpansion_samIncomingDamage(playerIndex,
                                                  source != NULL ? source->origin : NULL, damage);
        damage = skillExpansion_gimliIncomingDamage(playerIndex, damage);
        damage = skillExpansion_gandalfIncomingDamage(
            playerIndex, source != NULL ? source->origin : NULL, damage);
        damage = skillExpansion_legolasIncomingDamage(playerIndex, damage);
        damage = skillExpansion_eowynIncomingDamage(
            playerIndex, source != NULL ? source->origin : NULL, damage);
        flinchOnDamage = skillExpansion_gimliFlinch(playerIndex, flinchOnDamage);
        flinchOnDamage = skillExpansion_legolasFlinch(playerIndex, flinchOnDamage);
        flinchOnDamage = skillExpansion_eowynFlinch(playerIndex, flinchOnDamage);
        if (damage <= 0)
        {
            return;
        }
        amount = -(damage > 32767 ? 32767 : damage);
    }
    kit = skillExpansion_getHeroKit(PLAYER(playerIndex).heroId);
    if (amount < 0 && kit != NULL && kit->incomingDamage != NULL)
    {
        const HackLibDamageSource *source = hackLib_currentDamageSource();
        s32 damage =
            kit->incomingDamage(playerIndex, source != NULL ? source->origin : NULL, -(s32)amount);

        if (damage <= 0)
        {
            return;
        }
        amount = -(damage > 32767 ? 32767 : damage);
    }
    s32 hpBefore = PLAYER(playerIndex).currentHp;
    HpChangeObservation observation = {currentHpObservation, 0, playerIndex};
    currentHpObservation = &observation;
    if (kit != NULL && kit->onHpChange != NULL)
    {
        kit->onHpChange(playerIndex, amount, flinchOnDamage);
    }
    else
    {
        skillExpansion_original_player_addHp(playerIndex, amount, flinchOnDamage);
    }
    currentHpObservation = observation.previous;
    s32 hpAfter = PLAYER(playerIndex).currentHp;
    s32 delta = hpBefore - hpAfter;
    s32 ownDamage = delta - observation.childDelta;
    if (hpBefore <= 0)
    {
        ownDamage = 0;
    }
    else if (ownDamage > hpBefore)
    {
        ownDamage = hpBefore;
    }
    if (incoming != NULL && ownDamage > 0)
    {
        incoming->damage += ownDamage;
        incoming->killed |= hpBefore > 0 && hpAfter <= 0;
    }
    for (HpChangeObservation *parent = observation.previous; parent != NULL;
         parent = parent->previous)
    {
        if (parent->playerIndex == playerIndex)
        {
            parent->childDelta += delta;
            break;
        }
    }
}

// Each original runs the 8-byte prologue that its hookReplace overwrote, then continues past it.
#define ORIGINAL_THUNK(fn)                                                                         \
    asm(".section .text\n"                                                                         \
        ".balign 4\n"                                                                              \
        ".thumb\n"                                                                                 \
        ".global skillExpansion_original_" #fn "\n"                                                \
        ".thumb_func\n"                                                                            \
        "skillExpansion_original_" #fn ":\n"                                                       \
        "    push {r4-r7, lr}\n"                                                                   \
        "    mov  r7, r10\n"                                                                       \
        "    mov  r6, r9\n"                                                                        \
        "    mov  r5, r8\n"                                                                        \
        "    ldr  r3, =" #fn " + 8\n"                                                              \
        "    bx   r3\n"                                                                            \
        "    .pool\n")

ORIGINAL_THUNK(combat_computePveDamage);
ORIGINAL_THUNK(combat_resolvePveAttack);
ORIGINAL_THUNK(combat_applyPveRangedHit);
ORIGINAL_THUNK(combat_resolveEnemyAttackOnPlayer);

// hookReplaceR3 covers 16 bytes here. Preserve rawDamage in r3 while replaying that entry.
asm(".section .text\n.balign 4\n.thumb\n"
    ".global skillExpansion_original_enemyStatusArmor\n.thumb_func\n"
    "skillExpansion_original_enemyStatusArmor:\n"
    "push {r4-r7,lr}\nmov r7,r10\nmov r6,r9\nmov r5,r8\n"
    "push {r5-r7}\nsub sp,#4\nmov r7,r1\nmov r8,r2\n"
    "ldr r4, =combat_handleEnemyAttackOnPlayerStatusArmorSfx + 16\nbx r4\n.pool\n");

u16 skillExpansion_original_enemyStatusArmor(Actor *attacker, Actor *victim, u32 *outHitSfx,
                                             s16 rawDamage);

u16 skillExpansion_handleEnemyStatusArmor(Actor *attacker, Actor *victim, u32 *outHitSfx,
                                          u32 rawDamage)
{
    u8 p = victim->as.combat.playerIndex;
    Actor *shield = PLAYER(p).shieldBubble;
    bool poisonApplies = (EnemyTypeInfos[attacker->unitType].attackFlags.p & ENEMY_ATTACK_POISON) &&
                         !(PLAYER(p).heroId == HERO_ID_GANDALF && shield != NULL &&
                           shield->as.shieldGandalf.shieldHealth >= (s16)rawDamage);
    u16 armor =
        skillExpansion_original_enemyStatusArmor(attacker, victim, outHitSfx, (s16)rawDamage);
    IncomingResolution *resolution = currentIncomingResolution;
    if (resolution != NULL && resolution->victim == victim &&
        resolution->source == hackLib_currentDamageSource() && poisonApplies &&
        victim->as.combat.venomDuration > 0)
    {
        resolution->appliedStatusElements |= HACKLIB_ELEMENT_POISON;
    }
    return armor;
}
ORIGINAL_THUNK(actor_player_tick);

s32 skillExpansion_original_combat_computePveDamage(Actor *attacker, Actor *victim, bool crit);
bool skillExpansion_original_combat_resolvePveAttack(Actor *attacker, Actor *victim);
bool skillExpansion_original_combat_applyPveRangedHit(Actor *attacker, Actor *victim,
                                                      u16 baseDamage);
bool skillExpansion_original_combat_resolveEnemyAttackOnPlayer(Actor *attacker, Actor *victim);
void skillExpansion_original_actor_player_tick(Actor *actor);

bool32 skillExpansion_isPlayerAvatar(Actor *actor)
{
    if (actor == NULL)
    {
        return FALSE;
    }
    u8 p = actor->as.combat.playerIndex;

    return p < 2 && PLAYER(p).ownerActor == actor;
}

/**
 * A living, hittable enemy for the elite AoE effects. Enemies climbing in from a pillar or ledge
 * have no collision until they land, and a hit mid-climb strands them inside the wall.
 */
bool32 skillExpansion_isFoe(Actor *actor)
{
    return actor != NULL && !skillExpansion_isPlayerAvatar(actor) &&
           actor->type != ACTOR_TYPE_NPC && actor->currentHp > 0 &&
           (actor->flags.p & ACTOR_FLAG_COLLISION_CHECK) &&
           !(actor->flags.p & ACTOR_FLAG_INVULNERABLE);
}

static inline s32 passiveLevel(u8 playerIndex, u8 passiveSkillId)
{
    return skillExpansion_getPassiveLevel(playerIndex, passiveSkillId);
}

static inline bool32 isAragorn(u8 playerIndex)
{
    return PLAYER(playerIndex).heroId == HERO_ID_ARAGORN && skillExpansion_playerExpanded(playerIndex);
}

static bool isMarked(const EliteState *st, Actor *target)
{
    return st->markFrames != 0 && st->markTarget.actor == target &&
           st->markTarget.generation != 0 &&
           st->markTarget.generation == hackLib_actorGeneration(target);
}

/** Feed Anduril's Flame. It can't build while Flame of the West burns. */
static void stokeFlame(u8 playerIndex, Actor *playerActor, s32 amount)
{
    EliteState *st = &states[playerIndex];
    s32 meter;

    if (!isAragorn(playerIndex) || st->flameFrames != 0)
    {
        return;
    }
    st->meterIdleFrames = 0;
    meter = st->meter + amount;
    if (meter >= METER_MAX)
    {
        meter = METER_MAX;
#if HACK_gauntlet
        if (st->meter < METER_MAX)
        {
            gauntletBoons_flameFilled(playerIndex);
        }
#endif
        if (st->meter < METER_MAX && playerActor != NULL)
        {
            hackLib_fxBurst(playerActor, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
            sfx_play(SFX_COMBAT_SUNBURST);
        }
    }
    st->meter = meter;
}

static void dampFlame(EliteState *st, s32 amount)
{
    st->meter = st->meter > amount ? st->meter - amount : 0;
}

static bool32 isMarkTarget(Actor *target, u32 argument)
{
    (void)argument;
    return skillExpansion_isFoe(target);
}

static Actor *nearestMarkTarget(Actor *playerActor)
{
    HackLibTargetQuery query = {
        .center = playerActor, .filter = isMarkTarget, .radius = MARK_RADIUS};

    return hackLib_nearestTarget(&query);
}

static inline u8 castLevel(u8 playerIndex)
{
    return skillExpansion_effectiveActiveLevel(playerIndex, PLAYER(playerIndex).castingActiveSkill);
}

void skillExpansion_spawnBurstFx(Actor *target, u8 variant, s32 sfx)
{
    Actor *fx = fx_spawnOnActor(target, FX_TYPE_AOE_BURST, variant, 0);

    if (fx != NULL)
    {
        fx->type = FX_BURST_UNK_PARAM;
        fx->tickCallback = (ActorCallback)(void *)fx_tickAttachment;
    }
    sfx_play(sfx);
}

/** The stock Fireburst hit flash and sound. */
static void chargedFireFx(Actor *victim)
{
    Actor *fx = fx_spawnOnActor(victim, FX_TYPE_HIT_BURST, FX_BURST_FIRE, 1);

    if (fx != NULL)
    {
        fx->type = FX_BURST_UNK_PARAM;
        fx->tickCallback = (ActorCallback)(void *)fx_tickAttachment;
    }
    sfx_play(SFX_COMBAT_FIREBURST_PROC);
}

static void stunFoe(u8 playerIndex, Actor *foe, s32 frames)
{
    if (!skillExpansion_isFoe(foe) || !actor_isStunnable(foe))
    {
        return;
    }
    frames += PRESENCE_STUN_FRAMES_PER_LEVEL *
              passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_KINGS_PRESENCE);
    hackLib_applyStun(foe, frames);
}

static Actor *nextOwnedGhost(Actor *playerActor, Actor *cursor)
{
    return hackLib_nextOwnedGhost(playerActor, cursor);
}

/** Burst one ghost: hit and stun the foes around it, then dismiss it. */
static void ghostBurst(u8 playerIndex, Actor *playerActor, Actor *ghost, s32 damage, s32 stunFrames,
                       s32 meter)
{
    Actor *target = NULL;

    while ((target = hackLib_nextTargetAt(&ghost->coords, NULL, OATH_RADIUS, target)) != NULL)
    {
        if (skillExpansion_isFoe(target))
        {
            skillExpansion_applyPveRangedHit(playerActor, target, damage);
            stunFoe(playerIndex, target, stunFrames);
        }
    }
    skillExpansion_spawnBurstFx(ghost, FX_AOE_BURST_NIGHT, SFX_COMBAT_TRIPLE_DAMAGE);
    ghostState(ghost)->lifetime = 1;
    stokeFlame(playerIndex, playerActor, meter);
}

/** Raise one Call of the Dead ghost near the player, attacking an on-screen enemy. */
static Actor *raiseGhost(u8 playerIndex, Actor *playerActor, Actor *exclude)
{
    Actor *target = NULL;
    Actor *ghost;
    Vector2Fp16 pos;
    u8 level;

    do
    {
        target = findNextEnemy(playerActor, target);
    } while (target != NULL && (target == exclude || target->currentHp <= 0));
    if (target == NULL)
    {
        return NULL;
    }

    pos.x = playerActor->coords.x + (rng_rollRange(-GHOST_SPAWN_SPREAD, GHOST_SPAWN_SPREAD) << 16);
    pos.y = playerActor->coords.y + (rng_rollRange(-GHOST_SPAWN_SPREAD, GHOST_SPAWN_SPREAD) << 16);
    ghost = spawnNpc(GHOST_NPC_KIND, &pos, playerActor, target, GHOST_LIFETIME_FRAMES);
    if (ghost == NULL)
    {
        return NULL;
    }

    level = PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_ARAGORN_CALL_OF_THE_DEAD];
    ghostState(ghost)->damage = skill_active_getLeveledValue(
        playerIndex, ACTIVE_SKILL_ARAGORN_CALL_OF_THE_DEAD, GHOST_DAMAGE_VALUE, level);
    playerActor->as.combat.comboState[0]++;
    skillExpansion_trackGhost(ghost);
    return ghost;
}

static void endAmbush(EliteState *st, Actor *playerActor)
{
    st->ambushFrames = 0;
    playerActor->spriteFlags &= ~SPRITE_FLAG_STEALTH;
    playerActor->flags.p &= ~ACTOR_FLAG_23;
}

static void endReturnOfTheKing(u8 playerIndex, EliteState *st)
{
    u32 row;

    st->returnFrames = 0;
    skillExpansion_removeTemporaryLevels(playerIndex, RETURN_TEMPORARY_LEVELS);
    for (row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        skillExpansion_players[playerIndex].activeSkills[row].spiritCost =
            st->savedSpiritCosts[row];
    }
}

/** Extra damage from elite passives and buffs on a hit dealing damage. */
static s32 bonusDamage(u8 playerIndex, Actor *victim, s32 damage)
{
    EliteState *st = &states[playerIndex];
    s32 percent = 0;
    s32 bonus = 0;
    s32 level;

    if (st->patienceStrike)
    {
        percent += PATIENCE_PERCENT_PER_LEVEL *
                   passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_HUNTERS_PATIENCE);
    }
    if (isMarked(st, victim))
    {
        percent += MARK_BASE_PERCENT + MARK_PERCENT_PER_LEVEL * st->markLevel;
    }
    if (isAragorn(playerIndex) && st->meter >= METER_MAX)
    {
        percent += REFORGED_PERCENT;
    }

    level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_KINGS_PRESENCE);
    if (level != 0 && victim->actionState == ACTOR_STATE_STUNNED)
    {
        bonus += PRESENCE_DAMAGE_PER_LEVEL * level;
    }

    return damage * percent / 100 + bonus;
}

/** Count a kill toward the chain. The Dead Ride raises a ghost every few chained kills. */
static void chainKill(u8 playerIndex, Actor *playerActor, Actor *victim)
{
    EliteState *st = &states[playerIndex];
    s32 level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_THE_DEAD_RIDE);

    if (st->chainKills < 0xff)
    {
        st->chainKills++;
    }
    st->chainFrames = CHAIN_WINDOW_FRAMES;
    if (level != 0 && st->chainKills % (DEAD_RIDE_BASE_KILLS - level) == 0 &&
        playerActor->as.combat.comboState[0] < DEAD_RIDE_MAX_GHOSTS &&
        raiseGhost(playerIndex, playerActor, victim) != NULL)
    {
        hackLib_fxBurst(playerActor, FX_SHAPE_RISING_STREAKS, FX_COLOR_GHOST, FX_LAYER_FRONT);
    }
}

/** Hands of the King: heal you and your co-op partner. */
static void handsOfTheKing(u8 playerIndex, s32 level)
{
    u32 p;

    for (p = 0; p < 2; p++)
    {
        s32 heal;

        if (p != playerIndex && !(g_GameFlags.p & GAME_FLAG_COOP_ACTIVE))
        {
            continue;
        }
        if (PLAYER(p).ownerActor == NULL || PLAYER(p).currentHp <= 0)
        {
            continue;
        }
        heal = PLAYER_STAT(p, STAT_MAX_HP) * HANDS_PERCENT_PER_LEVEL * level / 100;
        player_addHp(p, heal > 0 ? heal : 1, FALSE);
    }
}

static void onKill(u8 playerIndex, Actor *playerActor, Actor *victim)
{
    EliteState *st = &states[playerIndex];
    s32 level;

    chainKill(playerIndex, playerActor, victim);
    if (st->flameFrames != 0)
    {
        st->flameFrames += FLAME_KILL_FRAMES;
        level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_HANDS_OF_THE_KING);
        if (level != 0)
        {
            handsOfTheKing(playerIndex, level);
        }
    }
    stokeFlame(playerIndex, playerActor, METER_KILL);
    if (isMarked(st, victim))
    {
        Actor *next = NULL;

        level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_TRACK_THE_QUARRY);
        if (level != 0)
        {
            next = nearestMarkTarget(playerActor);
        }
        if (next != NULL)
        {
            // Track the Quarry: the Mark leaps to the next foe.
            st->markTarget = hackLib_actorRef(next);
            st->markFrames += QUARRY_FRAMES_PER_LEVEL * level;
            hackLib_fxBurst(next, FX_SHAPE_DIAMOND_RING, FX_COLOR_GREEN, FX_LAYER_FRONT);
        }
        else
        {
            st->markFrames = 0;
            st->markTarget = (HackLibActorRef){0};
        }
        stokeFlame(playerIndex, playerActor, METER_MARK_KILL);
        hackLib_fxBurst(victim, FX_SHAPE_SPARKLE_RING, FX_COLOR_GREEN, FX_LAYER_FRONT);
    }

    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_LORD_OF_THE_DEAD) != 0 &&
        playerActor->as.combat.comboState[0] < LORD_MAX_GHOSTS &&
        rng_rollRange(1, 100) <= LORD_PERCENT)
    {
        raiseGhost(playerIndex, playerActor, victim);
    }
}

/** Flame of the West: scorch the foes around `center`. */
static void flameBurst(Actor *attacker, Actor *center, s32 damage)
{
    Actor *target = NULL;
    bool32 burned = FALSE;

    while ((target = hackLib_nextTargetAt(&center->coords, center, BURST_RADIUS, target)) != NULL)
    {
        if (target != center && skillExpansion_isFoe(target))
        {
            skillExpansion_dealDamage(attacker, target, damage, HACKLIB_DAMAGE_SECONDARY,
                                      HACKLIB_ABILITY_NONE);
            burned = TRUE;
        }
    }
    if (burned)
    {
        skillExpansion_spawnBurstFx(center, FX_AOE_BURST_SUN, SFX_COMBAT_SUNBURST);
    }
}

/** A blazing blow burns the foe it lands on and the foes around it. */
static void flameStrike(Actor *attacker, Actor *victim, s32 damage)
{
    if (victim->currentHp > 0)
    {
        skillExpansion_dealElementalDamage(attacker, victim, damage, HACKLIB_DAMAGE_SECONDARY,
                                           HACKLIB_ABILITY_NONE, HACKLIB_ELEMENT_FIRE);
    }
    flameBurst(attacker, victim, damage);
}

/** Strider's Riposte: counter every foe near the player after an enemy attack fails. */
static void stridersRiposte(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    s32 level = passiveLevel(p, SKILL_EXPANSION_PASSIVE_STRIDERS_RIPOSTE);
    Actor *target = NULL;

    if (level == 0 || rng_rollRange(1, 100) > RIPOSTE_PERCENT_PER_LEVEL * level)
    {
        return;
    }
    sfx_play(SFX_COMBAT_RIPOSTE);
    stokeFlame(p, playerActor, METER_RIPOSTE);
    while ((target = hackLib_nextTargetAt(&playerActor->coords, playerActor, RIPOSTE_RADIUS,
                                          target)) != NULL)
    {
        if (skillExpansion_isFoe(target))
        {
            skillExpansion_resolvePveAttack(playerActor, target);
        }
    }
}

/** Restless Dead: ghosts about to fade burst instead. */
static void restlessDead(u8 playerIndex, Actor *playerActor)
{
    s32 level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_RESTLESS_DEAD);
    Actor *ghost = NULL;

    if (level == 0)
    {
        return;
    }
    while ((ghost = nextOwnedGhost(playerActor, ghost)) != NULL)
    {
        if (ghostState(ghost)->lifetime <= RESTLESS_EXPIRY_FRAMES)
        {
            ghostBurst(playerIndex, playerActor, ghost,
                       RESTLESS_BASE_DAMAGE + RESTLESS_DAMAGE_PER_LEVEL * level,
                       RESTLESS_STUN_FRAMES, METER_RESTLESS);
        }
    }
}

/** The Blade Reforged: fill Anduril's Flame when health falls low. */
static void bladeReforged(u8 playerIndex, EliteState *st, Actor *playerActor)
{
    s32 level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_BLADE_REFORGED);
    s32 hp = PLAYER(playerIndex).currentHp;

    if (st->bladeCooldown != 0)
    {
        st->bladeCooldown--;
        return;
    }
    if (level == 0 || hp <= 0 ||
        hp * 100 >= PLAYER_STAT(playerIndex, STAT_MAX_HP) * BLADE_HP_PERCENT)
    {
        return;
    }
    st->bladeCooldown = BLADE_BASE_COOLDOWN_FRAMES - BLADE_COOLDOWN_FRAMES_PER_LEVEL * level;
    if (st->flameFrames != 0)
    {
        st->flameFrames += BLADE_FLAME_FRAMES;
    }
    else
    {
        stokeFlame(playerIndex, playerActor, METER_MAX);
    }
    hackLib_fxBurst(playerActor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_WHITE, FX_LAYER_FRONT);
}

/** Hunter's Patience: glint once Aragorn has held his blow long enough. */
static void huntersPatience(u8 playerIndex, EliteState *st, Actor *playerActor)
{
    if (st->patienceFrames < PATIENCE_READY_FRAMES &&
        ++st->patienceFrames == PATIENCE_READY_FRAMES &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_HUNTERS_PATIENCE) != 0)
    {
        hackLib_fxBurst(playerActor, FX_SHAPE_SPARK, FX_COLOR_GREEN, FX_LAYER_FRONT);
    }
}

/** Anduril's blade glows with a stock affix glow while the Flame bar is full or Flame of the West
 * burns. */
static void chargedBlade(u8 playerIndex, EliteState *st)
{
    Player *player = &PLAYER(playerIndex);
    Item weapon = player->inventory.array[WEAPON_INVENTORY_SLOT];
    u8 glow = 0;

    if (weapon.d.itemType <= ITEM_TYPE_MACE)
    {
        if (st->flameFrames != 0)
        {
            glow = BLAZE_GLOW_PREFIX;
        }
        else if (st->meter >= METER_MAX)
        {
            glow = CHARGED_GLOW_PREFIX;
        }
    }
    if (glow != 0 && glow != st->chargedGlow &&
        (st->chargedGlow != 0 || player->handFxEmitters[WEAPON_HAND] == NULL))
    {
        weapon.word = (weapon.word & ~0xff0000u) | ((u32)glow << 16);
        player_setHandFx(playerIndex, WEAPON_HAND, weapon);
        st->chargedGlow = glow;
    }
    else if (glow == 0 && st->chargedGlow != 0)
    {
        st->chargedGlow = 0;
        player->combatFlags.p |= PLAYER_COMBAT_FLAG_HAND_SPRITES_DIRTY;
    }
}

static void eliteTick(u8 playerIndex, Actor *playerActor)
{
    EliteState *st = &states[playerIndex];

    if (st->flameFrames != 0)
    {
        st->flameFrames--;
    }
    if (st->ambushFrames != 0 && --st->ambushFrames == 0)
    {
        endAmbush(st, playerActor);
    }
    if (st->markFrames != 0)
    {
        Actor *mark = st->markTarget.actor;

        if (--st->markFrames == 0 || !hackLib_isActorRefValid(st->markTarget) ||
            mark->currentHp <= 0 || !skillExpansion_isFoe(mark))
        {
            st->markFrames = 0;
            st->markTarget = (HackLibActorRef){0};
        }
        else if (st->markFrames % MARK_FX_FRAMES == 0)
        {
            hackLib_fxBurst(mark, FX_SHAPE_SPARK, FX_COLOR_GREEN, FX_LAYER_FRONT);
        }
    }
    if (st->willFrames != 0)
    {
        st->willFrames--;
    }
    if (isAragorn(playerIndex))
    {
        // Numenorean Will keeps wounds from dampening the Flame.
        if (PLAYER(playerIndex).currentHp < st->lastHp && st->willFrames == 0)
        {
            dampFlame(st, METER_HURT);
        }
        bladeReforged(playerIndex, st, playerActor);
        huntersPatience(playerIndex, st, playerActor);
        restlessDead(playerIndex, playerActor);
        chargedBlade(playerIndex, st);
        if (st->meterIdleFrames < METER_IDLE_FRAMES)
        {
            st->meterIdleFrames++;
        }
        else if (st->meter != 0 && st->meter < METER_MAX &&
                 ++st->meterIdleFrames >= METER_IDLE_FRAMES + METER_DECAY_FRAMES)
        {
            st->meterIdleFrames = METER_IDLE_FRAMES;
            st->meter--;
        }
    }
    if (st->returnFrames != 0 && --st->returnFrames == 0)
    {
        endReturnOfTheKing(playerIndex, st);
    }
    if (st->chainFrames != 0 && --st->chainFrames == 0)
    {
        st->chainKills = 0;
    }
    st->lastHp = PLAYER(playerIndex).currentHp;
    skillExpansion_frodoTick(playerIndex, playerActor);
    skillExpansion_smeagolTick(playerIndex, playerActor);
    skillExpansion_samTick(playerIndex, playerActor);
    skillExpansion_gimliTick(playerIndex, playerActor);
    skillExpansion_gandalfTick(playerIndex, playerActor);
    skillExpansion_legolasTick(playerIndex, playerActor);
    skillExpansion_eowynTick(playerIndex, playerActor);
}

void skillExpansion_eliteReset(u8 playerIndex, bool preserveFlame)
{
    u8 meter = preserveFlame && isAragorn(playerIndex) ? states[playerIndex].meter : 0;
    u16 idle = preserveFlame && isAragorn(playerIndex) ? states[playerIndex].meterIdleFrames : 0;

    memory_memset(&states[playerIndex], 0, sizeof(states[playerIndex]));
    states[playerIndex].meter = meter;
    states[playerIndex].meterIdleFrames = idle;
    skillExpansion_frodoReset(playerIndex);
    skillExpansion_smeagolReset(playerIndex);
    skillExpansion_samReset(playerIndex);
    skillExpansion_gimliReset(playerIndex);
    skillExpansion_gandalfReset(playerIndex);
    skillExpansion_legolasReset(playerIndex);
    skillExpansion_eowynReset(playerIndex);
}

void skillExpansion_eliteCleanup(u8 playerIndex, Actor *actor)
{
    skillExpansion_frodoCleanup(playerIndex, actor);
    skillExpansion_smeagolCleanup(playerIndex, actor);
    skillExpansion_samCleanup(playerIndex, actor);
    skillExpansion_gimliCleanup(playerIndex, actor);
    skillExpansion_gandalfCleanup(playerIndex, actor);
    skillExpansion_legolasCleanup(playerIndex, actor);
    skillExpansion_eowynCleanup(playerIndex, actor);
    if (states[playerIndex].ambushFrames != 0)
    {
        endAmbush(&states[playerIndex], actor);
    }
}

typedef struct MeleeResolution
{
    Actor *attacker;
    Actor *victim;
    bool criticalKnown;
    bool critical;
    u8 elements;
    bool elementsKnown;
} MeleeResolution;

static MeleeResolution *currentMeleeResolution;

/** Melee bonuses around combat_computePveDamage, retaining its native crit result. */
s32 skillExpansion_computePveDamage(Actor *attacker, Actor *victim, bool crit)
{
    if (currentMeleeResolution != NULL && currentMeleeResolution->attacker == attacker &&
        currentMeleeResolution->victim == victim)
    {
        currentMeleeResolution->criticalKnown = TRUE;
        currentMeleeResolution->critical = crit;
        u8 p = attacker->as.combat.playerIndex;
        currentMeleeResolution->elements =
            (PLAYER_STAT(p, STAT_FIREBURST) ? HACKLIB_ELEMENT_FIRE : 0) |
            (PLAYER_STAT(p, STAT_VENOMOUS) ? HACKLIB_ELEMENT_POISON : 0);
        currentMeleeResolution->elementsKnown = TRUE;
    }
    s32 damage = skillExpansion_original_combat_computePveDamage(attacker, victim, crit);
    u8 p;

    if (!skillExpansion_isPlayerAvatar(attacker))
    {
        return damage;
    }
    p = attacker->as.combat.playerIndex;
    if (states[p].ambushStrike && crit)
    {
        damage =
            damage *
            (AMBUSH_CRIT_BASE_PERCENT + AMBUSH_CRIT_PERCENT_PER_LEVEL * states[p].ambushLevel) /
            100;
    }
    damage += bonusDamage(p, victim, damage);
    damage = skillExpansion_frodoDamage(p, damage, TRUE);
    damage = skillExpansion_smeagolDamage(p, damage, TRUE);
    damage = skillExpansion_samDamage(p, damage, TRUE);
    damage = skillExpansion_gimliDamage(p, victim, damage);
    damage = skillExpansion_gandalfDamage(p, victim, damage, TRUE);
    damage = skillExpansion_eowynDamage(p, victim, damage, TRUE);
    {
        const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(PLAYER(p).heroId);
        if (kit != NULL && kit->meleeDamage != NULL)
        {
            damage = kit->meleeDamage(p, victim, damage);
        }
    }
#if HACK_twists
    damage = twists_damageDealt(p, damage);
#endif
#if HACK_gauntlet
    damage = gauntlet_damageDealt(p, victim, damage);
#endif
#if HACK_debug_cheats
    damage = debugCheats_meleeDamage(victim, damage);
#endif
    return (u16)damage;
}

typedef struct CombatEventContext
{
    u8 playerIndex;
    const SkillExpansionHeroKit *kit;
} CombatEventContext;

static void combatKill(const HackLibCombatEvent *event, void *context)
{
    CombatEventContext *dispatch = context;
    u8 p = dispatch->playerIndex;
    onKill(p, event->attacker, event->victim);
    skillExpansion_gimliOnKill(p, event->victim);
    skillExpansion_gandalfOnKill(p, event->victim);
    if (dispatch->kit != NULL && dispatch->kit->onKill != NULL)
    {
        dispatch->kit->onKill(p, event->attacker, event->victim);
    }
}

static void combatHit(const HackLibCombatEvent *event, void *context)
{
    CombatEventContext *dispatch = context;
    u8 p = dispatch->playerIndex;
    if (dispatch->kit != NULL && dispatch->kit->onHit != NULL)
    {
        dispatch->kit->onHit(p, event->attacker, event->victim, event->result, event->melee);
    }
    skillExpansion_frodoOnHit(p, event->victim, event->result, event->melee);
    skillExpansion_smeagolOnHit(p, event->victim, event->result, event->melee);
    skillExpansion_eowynOnHit(p, event->attacker, event->victim, event->result, event->melee);
}

static void combatResolved(const HackLibCombatEvent *event, void *context)
{
    CombatEventContext *dispatch = context;
    if (dispatch->kit != NULL && dispatch->kit->onCombatEvent != NULL)
    {
        dispatch->kit->onCombatEvent(dispatch->playerIndex, event);
    }
}

static void dispatchCombatEvent(u8 playerIndex, const HackLibCombatEvent *event)
{
    static const HackLibCombatHandlers handlers = {combatKill, combatHit, combatResolved};
    CombatEventContext context = {playerIndex,
                                  skillExpansion_getHeroKit(PLAYER(playerIndex).heroId)};
    hackLib_dispatchCombatEvent(event, &handlers, &context);
}

static void incomingCombatResolved(const HackLibCombatEvent *event, void *context)
{
    CombatEventContext *dispatch = context;
    if (dispatch->kit != NULL && dispatch->kit->onIncomingCombatEvent != NULL)
    {
        dispatch->kit->onIncomingCombatEvent(dispatch->playerIndex, event);
    }
}

static void dispatchIncomingCombatEvent(u8 playerIndex, const HackLibCombatEvent *event)
{
    static const HackLibCombatHandlers handlers = {NULL, NULL, incomingCombatResolved};
    CombatEventContext context = {playerIndex,
                                  skillExpansion_getHeroKit(PLAYER(playerIndex).heroId)};
    hackLib_dispatchCombatEvent(event, &handlers, &context);
}

/** Melee bonuses around combat_resolvePveAttack. */
static bool resolveMeleeHit(Actor *attacker, Actor *victim)
{
    s16 hpBefore = victim->currentHp;
    HackLibDamageResult result;
    u8 stateBefore = victim->actionState;
    PlayerCombatFlags *combatFlags;
    EliteState *st;
    bool setPool = FALSE;
    bool hit;
    u8 p;

    if (!skillExpansion_isPlayerAvatar(attacker))
    {
        return skillExpansion_original_combat_resolvePveAttack(attacker, victim);
    }
    p = attacker->as.combat.playerIndex;
    st = &states[p];
    combatFlags = &PLAYER(p).combatFlags;

    if (st->ambushFrames != 0)
    {
        st->ambushStrike = TRUE;
        if (!(combatFlags->p & PLAYER_COMBAT_FLAG_WARRIORS_POOL))
        {
            combatFlags->p |= PLAYER_COMBAT_FLAG_WARRIORS_POOL;
            setPool = TRUE;
        }
    }
    st->patienceStrike = st->patienceFrames >= PATIENCE_READY_FRAMES &&
                         passiveLevel(p, SKILL_EXPANSION_PASSIVE_HUNTERS_PATIENCE) != 0;
    MeleeResolution resolution = {attacker, victim, FALSE, FALSE, HACKLIB_ELEMENT_NONE, FALSE};
    MeleeResolution *previousResolution = currentMeleeResolution;
    currentMeleeResolution = &resolution;
    hit = skillExpansion_original_combat_resolvePveAttack(attacker, victim);
    currentMeleeResolution = previousResolution;
    result = hackLib_damageResult(hpBefore, victim->currentHp, hit);
    HackLibCombatEvent event = hackLib_captureCombatEvent(attacker, victim, result, TRUE);
    event.criticalKnown = resolution.criticalKnown;
    event.critical = resolution.critical;
    if (resolution.elementsKnown)
    {
        event.source.elements =
            resolution.elements | (event.source.elementsKnown ? event.source.elements : 0);
        event.source.elementsKnown = TRUE;
    }
#if HACK_debug_cheats
    debugLab_recordCombatEvent(p, &event);
#endif
#if HACK_twists
    twists_combatEvent(p, &event);
#endif
#if HACK_gauntlet
    gauntlet_combatEvent(p, &event);
#endif
    st->ambushStrike = FALSE;
    st->patienceFrames = 0;
    if (st->patienceStrike && hit)
    {
        stokeFlame(p, attacker, METER_PATIENCE);
        hackLib_fxBurst(victim, FX_SHAPE_SPARK, FX_COLOR_GREEN, FX_LAYER_FRONT);
    }
    st->patienceStrike = FALSE;
    if (setPool)
    {
        combatFlags->p &= ~PLAYER_COMBAT_FLAG_WARRIORS_POOL;
    }
    if (!hit)
    {
        dispatchCombatEvent(p, &event);
        return hit;
    }

    if (st->meter >= METER_MAX && st->flameFrames == 0 && victim->currentHp > 0)
    {
        skillExpansion_dealElementalDamage(attacker, victim, CHARGED_FIRE_DAMAGE,
                                           HACKLIB_DAMAGE_SECONDARY, HACKLIB_ABILITY_NONE,
                                           HACKLIB_ELEMENT_FIRE);
        chargedFireFx(victim);
    }
    stokeFlame(p, attacker, isMarked(st, victim) ? 2 * METER_HIT : METER_HIT);
    if (stateBefore != ACTOR_STATE_STUNNED && victim->actionState == ACTOR_STATE_STUNNED)
    {
        victim->actionStateTimer += PRESENCE_STUN_FRAMES_PER_LEVEL *
                                    passiveLevel(p, SKILL_EXPANSION_PASSIVE_KINGS_PRESENCE);
        stokeFlame(p, attacker, METER_STUN);
    }
    if (st->ambushFrames != 0)
    {
        endAmbush(st, attacker);
        stunFoe(p, victim, AMBUSH_STUN_FRAMES);
        stokeFlame(p, attacker, METER_AMBUSH);
    }
    dispatchCombatEvent(p, &event);
    if (st->flameFrames != 0)
    {
        s32 damage = FLAME_BURST_BASE_DAMAGE + FLAME_BURST_DAMAGE_PER_LEVEL * st->flameLevel;

        flameStrike(attacker, victim, st->flameReforged ? 2 * damage : damage);
    }
    return hit;
}

bool skillExpansion_resolvePveAttack(Actor *attacker, Actor *victim)
{
    const HackLibDamageSource *parent = hackLib_currentDamageSource();
    HackLibDamageSource source = {attacker, HACKLIB_DAMAGE_WEAPON, HACKLIB_ABILITY_NONE,
                                  0,        HACKLIB_ELEMENT_NONE,  FALSE};
    bool hit;

    if (parent != NULL && (parent->kind != HACKLIB_DAMAGE_WEAPON || parent->origin == attacker))
    {
        source = *parent;
    }
    else if (parent != NULL)
    {
        source.depth =
            parent->depth < HACKLIB_MAX_DAMAGE_DEPTH ? parent->depth + 1 : HACKLIB_MAX_DAMAGE_DEPTH;
    }
    hackLib_swapDamageSource(&source);
    hit = resolveMeleeHit(attacker, victim);
    hackLib_swapDamageSource(parent);
    return hit;
}

/**
 * Projectile and ghost hits with the elite damage bonuses and kill tracking.
 *
 * WRAPS: combat_applyPveRangedHit.
 */
bool skillExpansion_applyPveRangedHit(Actor *attacker, Actor *victim, u16 baseDamage)
{
    return skillExpansion_applyRangedHitResult(attacker, victim, baseDamage).hit;
}

static HackLibDamageResult dealDamage(Actor *attacker, Actor *victim, u16 damage, u8 kind,
                                      u8 abilityRow, u8 elements, bool elementsKnown)
{
    const HackLibDamageSource *parent = hackLib_currentDamageSource();
    HackLibDamageSource source = {attacker,   kind,
                                  abilityRow, parent != NULL ? parent->depth + 1 : 0,
                                  elements,   elementsKnown};
    HackLibDamageResult result = {0};

    if (source.depth >= HACKLIB_MAX_DAMAGE_DEPTH)
    {
        return result;
    }
    hackLib_swapDamageSource(&source);
    result = skillExpansion_applyRangedHitResult(attacker, victim, damage);
    hackLib_swapDamageSource(parent);
    return result;
}

HackLibDamageResult skillExpansion_dealDamage(Actor *attacker, Actor *victim, u16 damage, u8 kind,
                                              u8 abilityRow)
{
    return dealDamage(attacker, victim, damage, kind, abilityRow, HACKLIB_ELEMENT_NONE, FALSE);
}

HackLibDamageResult skillExpansion_dealElementalDamage(Actor *attacker, Actor *victim, u16 damage,
                                                       u8 kind, u8 abilityRow, u8 elements)
{
    if (elements & ~HACKLIB_ELEMENT_MASK)
    {
        return (HackLibDamageResult){0};
    }
    return dealDamage(attacker, victim, damage, kind, abilityRow, elements, TRUE);
}

void skillExpansion_ghostTrigger(Actor *ghost, Actor *victim)
{
    HackLibDamageSource source = {ghost, HACKLIB_DAMAGE_SUMMON, HACKLIB_ABILITY_NONE,
                                  0,     HACKLIB_ELEMENT_NONE,  FALSE};
    const HackLibDamageSource *previous = hackLib_swapDamageSource(&source);
    Actor *owner = ghost->parentActor;
    s16 hpBefore = victim->currentHp;

    actor_npcGhost_trigger(ghost, victim);
    hackLib_swapDamageSource(previous);
    if (owner != NULL && skillExpansion_isPlayerAvatar(owner) && victim->currentHp < hpBefore)
    {
        u8 p = owner->as.combat.playerIndex;

        stokeFlame(p, owner, METER_GHOST_HIT);
        if (victim->currentHp <= 0 && hpBefore > 0)
        {
            chainKill(p, owner, victim);
        }
    }
}

static HackLibDamageResult resolveRangedHit(Actor *attacker, Actor *victim, u16 baseDamage)
{
    s16 hpBefore = victim->currentHp;
    HackLibDamageResult result;
    bool hit;
    u8 p;

    if (!skillExpansion_isPlayerAvatar(attacker))
    {
        hit = skillExpansion_original_combat_applyPveRangedHit(attacker, victim, baseDamage);
        return hackLib_damageResult(hpBefore, victim->currentHp, hit);
    }
    p = attacker->as.combat.playerIndex;
    baseDamage += bonusDamage(p, victim, baseDamage);
    baseDamage = skillExpansion_frodoDamage(p, baseDamage, FALSE);
    baseDamage = skillExpansion_smeagolDamage(p, baseDamage, FALSE);
    baseDamage = skillExpansion_samDamage(p, baseDamage, FALSE);
    baseDamage = skillExpansion_gimliDamage(p, victim, baseDamage);
    baseDamage = skillExpansion_gandalfDamage(p, victim, baseDamage, FALSE);
    baseDamage = skillExpansion_legolasDamage(p, victim, baseDamage, FALSE);
    baseDamage = skillExpansion_eowynDamage(p, victim, baseDamage, FALSE);
    {
        const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(PLAYER(p).heroId);
        if (kit != NULL && kit->rangedDamage != NULL)
        {
            baseDamage = kit->rangedDamage(p, victim, baseDamage);
        }
    }
#if HACK_twists
    baseDamage = twists_damageDealt(p, baseDamage);
#endif
#if HACK_gauntlet
    baseDamage = gauntlet_damageDealt(p, victim, baseDamage);
#endif
    hit = skillExpansion_original_combat_applyPveRangedHit(attacker, victim, baseDamage);
    result = hackLib_damageResult(hpBefore, victim->currentHp, hit);
    HackLibCombatEvent event = hackLib_captureCombatEvent(attacker, victim, result, FALSE);
#if HACK_debug_cheats
    debugLab_recordCombatEvent(p, &event);
#endif
#if HACK_twists
    twists_combatEvent(p, &event);
#endif
#if HACK_gauntlet
    gauntlet_combatEvent(p, &event);
#endif
    dispatchCombatEvent(p, &event);
    if (hit)
    {
        const HackLibDamageSource *source = hackLib_currentDamageSource();
        EliteState *st = &states[p];

        if (source != NULL && source->kind == HACKLIB_DAMAGE_PROJECTILE)
        {
            stokeFlame(p, attacker, METER_HIT);
            if (st->flameFrames != 0)
            {
                s32 damage =
                    FLAME_BURST_BASE_DAMAGE + FLAME_BURST_DAMAGE_PER_LEVEL * st->flameLevel;

                flameStrike(attacker, victim, st->flameReforged ? 2 * damage : damage);
            }
        }
    }
    return result;
}

HackLibDamageResult skillExpansion_applyRangedHitResult(Actor *attacker, Actor *victim,
                                                        u16 baseDamage)
{
    const HackLibDamageSource *parent = hackLib_currentDamageSource();
    HackLibDamageSource source = {attacker, HACKLIB_DAMAGE_PROJECTILE, HACKLIB_ABILITY_NONE,
                                  0,        HACKLIB_ELEMENT_NONE,      FALSE};
    HackLibDamageResult result;

    if (parent != NULL)
    {
        source = *parent;
    }
    hackLib_swapDamageSource(&source);
    result = resolveRangedHit(attacker, victim, baseDamage);
    hackLib_swapDamageSource(parent);
    return result;
}

void skillExpansion_trackGhost(Actor *ghost)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(ghost->triggerCallbacks); i++)
    {
        if (ghost->triggerCallbacks[i] == actor_npcGhost_trigger)
        {
            ghost->triggerCallbacks[i] = skillExpansion_ghostTrigger;
        }
    }
}

/**
 * Enemy attack on a player, with Strider's Riposte when it fails to land.
 *
 * WRAPS: combat_resolveEnemyAttackOnPlayer.
 */
bool skillExpansion_resolveEnemyAttackOnPlayer(Actor *attacker, Actor *victim)
{
    bool vulnerable = !(victim->flags.p & ACTOR_FLAG_INVULNERABLE);
    bool hit;
    bool playerVictim = skillExpansion_isPlayerAvatar(victim);
    u8 p = playerVictim ? victim->as.combat.playerIndex : 0;
    s32 hpBefore = playerVictim ? PLAYER(p).currentHp : 0;

    HackLibDamageSource source = {attacker, HACKLIB_DAMAGE_WEAPON, HACKLIB_ABILITY_NONE,
                                  0,        HACKLIB_ELEMENT_NONE,  FALSE};
    const HackLibDamageSource *previous = hackLib_swapDamageSource(&source);
    if (previous != NULL)
    {
        source.depth = previous->depth < HACKLIB_MAX_DAMAGE_DEPTH ? previous->depth + 1
                                                                  : HACKLIB_MAX_DAMAGE_DEPTH;
    }
#if HACK_ringwraith
    Actor *previousEnemyAttacker = ringwraith_kitEnemyAttacker;
    ringwraith_kitEnemyAttacker = attacker;
#endif
    source.elements =
        (EnemyTypeInfos[attacker->unitType].attackFlags.p & ENEMY_ATTACK_FIRE ? HACKLIB_ELEMENT_FIRE
                                                                              : 0) |
        (EnemyTypeInfos[attacker->unitType].attackFlags.p & ENEMY_ATTACK_POISON
             ? HACKLIB_ELEMENT_POISON
             : 0);
    source.elementsKnown = TRUE;
    IncomingResolution resolution = {&source, 0, p, FALSE, FALSE, victim, HACKLIB_ELEMENT_NONE};
    IncomingResolution *previousResolution = currentIncomingResolution;
    currentIncomingResolution = &resolution;
    hit = skillExpansion_original_combat_resolveEnemyAttackOnPlayer(attacker, victim);
    currentIncomingResolution = previousResolution;
#if HACK_twists
    if (playerVictim)
    {
        twists_enemyHitResolved(p, hpBefore);
    }
#endif
#if HACK_gauntlet
    if (playerVictim)
    {
        gauntlet_enemyHitResolved(p, hpBefore);
    }
#endif
    HackLibDamageResult result =
        hackLib_damageResult(hpBefore, playerVictim ? PLAYER(p).currentHp : 0, hit);
    if (resolution.observed && hit)
    {
        result.damageDealt = resolution.damage;
        result.killed = resolution.killed;
    }
    HackLibCombatEvent event = hackLib_captureCombatEvent(
        attacker, victim, result,
        !(EnemyTypeInfos[attacker->unitType].attackFlags.p & ENEMY_ATTACK_RANGED));
    event.appliedStatusElements = resolution.appliedStatusElements;
    event.appliedStatusElementsKnown = TRUE;
    hackLib_swapDamageSource(previous);
#if HACK_ringwraith
    ringwraith_kitEnemyAttacker = previousEnemyAttacker;
#endif

    if (playerVictim)
    {
        dispatchIncomingCombatEvent(p, &event);
    }

    if (!hit && vulnerable && skillExpansion_isPlayerAvatar(victim))
    {
        stridersRiposte(victim);
    }
    return hit;
}

/**
 * Per-frame elite timers and regeneration.
 *
 * WRAPS: actor_player_tick.
 */
void skillExpansion_playerTick(Actor *actor)
{
    HackLibProfileSpan profile = hackLib_profileBegin(HACKLIB_PROFILE_PLAYER);
    if (skillExpansion_isPlayerAvatar(actor))
    {
        HackLibProfileSpan passives = hackLib_profileBegin(HACKLIB_PROFILE_PASSIVES);
        eliteTick(actor->as.combat.playerIndex, actor);
        hackLib_profileEnd(passives);
        skillExpansion_runtimeTick(actor->as.combat.playerIndex, actor);
#if HACK_debug_cheats
        debugCheats_playerTick(actor->as.combat.playerIndex);
#endif
#if HACK_twists
        twists_playerTick(actor->as.combat.playerIndex);
#endif
#if HACK_gauntlet
        gauntlet_playerTick(actor->as.combat.playerIndex);
#endif
    }
    skillExpansion_original_actor_player_tick(actor);
    hackLib_profileEnd(profile);
}

/** Call of the Dead, tracking the raised ghosts so their hits stoke the Flame. */
static void callOfTheDead(Actor *playerActor)
{
    Actor *ghost = NULL;

    skill_active_aragornCallOfTheDead(playerActor);
    while ((ghost = nextOwnedGhost(playerActor, ghost)) != NULL)
    {
        skillExpansion_trackGhost(ghost);
    }
}

static void sweep(Actor *playerActor)
{
    skill_active_aragornSweep(playerActor);
    stokeFlame(playerActor->as.combat.playerIndex, playerActor, METER_SWEEP);
}

static void kingsCommand(Actor *playerActor)
{
    skill_active_aragornKingsCommand(playerActor);
    stokeFlame(playerActor->as.combat.playerIndex, playerActor, METER_KINGS_COMMAND);
}

static void numenoreanWill(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;

    skill_active_aragornNumenoreanWill(playerActor);
    states[p].willFrames = WILL_FRAMES_PER_LEVEL * castLevel(p);
}

/** Heal with Kingsfoil. Higher ranks add maximum-HP healing. */
static void herbalHealing(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    KingsfoilHerbs before = PLAYER(p).kingsfoilHerbs;

    skill_active_herbalHealing(playerActor);
    if (PLAYER(p).kingsfoilHerbs.fresh < before.fresh ||
        PLAYER(p).kingsfoilHerbs.dried < before.dried)
    {
        u32 level = skillExpansion_effectiveActiveLevel(p, ACTIVE_SKILL_HERBAL_HEALING);
        if (level > ACTIVE_SKILL_MAX_LEVEL)
        {
            level = ACTIVE_SKILL_MAX_LEVEL;
        }
        if (level > 1)
        {
            s32 bonus = PLAYER_STAT(p, STAT_MAX_HP) * (level - 1) / 10;
            s32 missingHp = PLAYER_STAT(p, STAT_MAX_HP) - PLAYER(p).currentHp;

            if (bonus > missingHp)
            {
                bonus = missingHp;
            }
            if (bonus > 0)
            {
                player_addHp(p, bonus, FALSE);
            }
        }
        stokeFlame(p, playerActor, METER_HERB);
    }
}

/** Point stock rows whose casts feed elite passives at their wrappers. */
void skillExpansion_wrapStockCallbacks(u32 heroId, u8 playerIndex)
{
    ActiveSkill *rows = skillExpansion_players[playerIndex].activeSkills;

    rows[ACTIVE_SKILL_HERBAL_HEALING].callback = herbalHealing;
    if (heroId == HERO_ID_ARAGORN)
    {
        rows[ACTIVE_SKILL_ARAGORN_SWEEP].callback = sweep;
        rows[ACTIVE_SKILL_ARAGORN_KINGS_COMMAND].callback = kingsCommand;
        rows[ACTIVE_SKILL_ARAGORN_NUMENOREAN_WILL].callback = numenoreanWill;
        rows[ACTIVE_SKILL_ARAGORN_CALL_OF_THE_DEAD].callback = callOfTheDead;
    }
    else if (heroId == HERO_ID_FRODO)
    {
        skillExpansion_frodoWrapStockCallbacks(playerIndex, rows);
    }
    else if (heroId == HERO_ID_EOWYN)
    {
        skillExpansion_eowynWrapStockCallbacks(playerIndex, rows);
    }
}

void skillExpansion_aragornFlameOfTheWest(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    EliteState *st = &states[p];

    st->flameLevel = castLevel(p);
    st->flameReforged = st->meter >= METER_MAX;
    st->flameFrames = FLAME_BASE_FRAMES + FLAME_FRAMES_PER_LEVEL * st->flameLevel +
                      FLAME_FULL_FRAMES * st->meter / METER_MAX;
    st->meter = 0;
    skillExpansion_spawnBurstFx(playerActor, FX_AOE_BURST_SUN, SFX_COMBAT_SUNBURST);
    if (st->flameReforged)
    {
        flameBurst(playerActor, playerActor,
                   FLAME_RING_BASE_DAMAGE + FLAME_RING_DAMAGE_PER_LEVEL * st->flameLevel);
        hackLib_fxBurst(playerActor, FX_SHAPE_WIDE_RING, FX_COLOR_EMBER, FX_LAYER_FRONT);
    }
}

void skillExpansion_aragornRangersAmbush(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    EliteState *st = &states[p];

    st->ambushLevel = castLevel(p);
    st->ambushFrames = AMBUSH_BASE_FRAMES + AMBUSH_FRAMES_PER_LEVEL * st->ambushLevel;
    playerActor->spriteFlags |= SPRITE_FLAG_STEALTH;
    playerActor->flags.p |= ACTOR_FLAG_23;
    sfx_play(AMBUSH_SFX);
}

void skillExpansion_aragornTrackersMark(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    EliteState *st = &states[p];
    Actor *target = nearestMarkTarget(playerActor);

    if (target == NULL)
    {
        return;
    }
    st->markTarget = hackLib_actorRef(target);
    st->markLevel = castLevel(p);
    st->markFrames = MARK_BASE_FRAMES + MARK_FRAMES_PER_LEVEL * st->markLevel;
    hackLib_fxBurst(target, FX_SHAPE_DIAMOND_RING, FX_COLOR_GREEN, FX_LAYER_FRONT);
    sfx_play(AMBUSH_SFX);
}

void skillExpansion_aragornOathOfTheDead(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    s32 damage = OATH_BASE_DAMAGE + OATH_DAMAGE_PER_LEVEL * castLevel(p);
    Actor *ghost = nextOwnedGhost(playerActor, NULL);

    if (ghost == NULL)
    {
        raiseGhost(p, playerActor, NULL);
        return;
    }
    for (; ghost != NULL; ghost = nextOwnedGhost(playerActor, ghost))
    {
        ghostBurst(p, playerActor, ghost, damage, OATH_STUN_FRAMES, METER_OATH_GHOST);
    }
}

void skillExpansion_aragornReturnOfTheKing(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    EliteState *st = &states[p];
    ActiveSkill *rows = skillExpansion_players[p].activeSkills;
    u32 row;

    st->returnFrames = RETURN_BASE_FRAMES + RETURN_FRAMES_PER_LEVEL * castLevel(p);
    if (st->flameFrames == 0)
    {
        st->meter = METER_MAX;
    }
    skillExpansion_addTemporaryLevels(p, RETURN_TEMPORARY_LEVELS);
    for (row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        st->savedSpiritCosts[row] = rows[row].spiritCost;
        rows[row].spiritCost = 0;
    }
    skillExpansion_spawnBurstFx(playerActor, FX_AOE_BURST_SUN, SFX_COMBAT_SUNBURST);
}

bool32 skillExpansion_canCastFlameOfTheWest(u8 playerIndex)
{
    return states[playerIndex].flameFrames == 0;
}

bool32 skillExpansion_canCastRangersAmbush(u8 playerIndex)
{
    return states[playerIndex].ambushFrames == 0;
}

bool32 skillExpansion_canCastTrackersMark(u8 playerIndex)
{
    Actor *playerActor = PLAYER(playerIndex).ownerActor;

    return states[playerIndex].markFrames == 0 && playerActor != NULL &&
           nearestMarkTarget(playerActor) != NULL;
}

bool32 skillExpansion_canCastReturnOfTheKing(u8 playerIndex)
{
    return states[playerIndex].returnFrames == 0;
}

static const HackLibHudMeterLook FlameBarCharge = {HACKLIB_HUD_COLOR_GOLD, HACKLIB_HUD_COLOR_EMBER,
                                                   HACKLIB_HUD_COLOR_DARK_RED,
                                                   HACKLIB_HUD_COLOR_WHITE};
static const HackLibHudMeterLook FlameBarBright = {HACKLIB_HUD_COLOR_PALE_GOLD,
                                                   HACKLIB_HUD_COLOR_GOLD, HACKLIB_HUD_COLOR_EMBER,
                                                   HACKLIB_HUD_COLOR_WHITE};
static const HackLibHudMeterLook FlameBarGlint = {HACKLIB_HUD_COLOR_WHITE,
                                                  HACKLIB_HUD_COLOR_PALE_GOLD,
                                                  HACKLIB_HUD_COLOR_RED, HACKLIB_HUD_COLOR_WHITE};
static const u8 FlameBarMarks[] = {METER_MAX / 2};

void skillExpansion_aragornDrawFlameBar(u8 playerIndex)
{
    EliteState *st = &states[playerIndex];
    s32 value = st->meter;
    const HackLibHudMeterLook *look;

    if (!isAragorn(playerIndex))
    {
        return;
    }
    if (++st->barPulseFrames >= 2 * BAR_PULSE_FRAMES)
    {
        st->barPulseFrames = 0;
    }
    if (value > st->barMeter)
    {
        st->barFlashFrames = BAR_FLASH_FRAMES;
    }
    st->barMeter = value;
    if (st->flameFrames != 0)
    {
        // The blaze burns the bar down as it runs.
        u32 total = FLAME_BASE_FRAMES + FLAME_FRAMES_PER_LEVEL * st->flameLevel + FLAME_FULL_FRAMES;

        value = st->flameFrames >= total ? METER_MAX : st->flameFrames * METER_MAX / total;
        look = st->barPulseFrames < BAR_PULSE_FRAMES ? &FlameBarBright : &FlameBarCharge;
    }
    else if (st->barFlashFrames != 0)
    {
        st->barFlashFrames--;
        look = &FlameBarBright;
    }
    else if (value >= METER_MAX)
    {
        look = st->barPulseFrames < BAR_PULSE_FRAMES ? &FlameBarGlint : &FlameBarBright;
    }
    else
    {
        look = &FlameBarCharge;
    }
    hackLib_drawHudMeter(&HackLibHudGapMeter, value, METER_MAX, look, FlameBarMarks,
                         ARRAY_COUNT(FlameBarMarks));
}
