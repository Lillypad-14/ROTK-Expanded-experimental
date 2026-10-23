#pragma once

// Elite (page 2) skill mechanics. Each elite active has its own cast callback and cast condition,
// and the elite passives act through the combat wrappers in skill_expansion_elite.c.

#include "actor.h"
#include "actor/npc.h"
#include "lib/combat_result.h"
#include "lib/combat_source.h"
#include "types.h"

#include "lib/summon.h"

// Call of the Dead ghosts
#define GHOST_LIFETIME_FRAMES 180
#define GHOST_SPAWN_SPREAD 60
#define GHOST_DAMAGE_VALUE 1

/** A player's avatar actor. */
bool32 skillExpansion_isPlayerAvatar(Actor *actor);

/** A living, hittable enemy for skill AoE effects. */
bool32 skillExpansion_isFoe(Actor *actor);

void skillExpansion_spawnBurstFx(Actor *target, u8 variant, s32 sfx);

bool skillExpansion_resolvePveAttack(Actor *attacker, Actor *victim);
bool skillExpansion_applyPveRangedHit(Actor *attacker, Actor *victim, u16 baseDamage);

/** Resolve ranged damage and capture its result before secondary effects run. */
HackLibDamageResult skillExpansion_applyRangedHitResult(Actor *attacker, Actor *victim,
                                                        u16 baseDamage);
HackLibDamageResult skillExpansion_dealDamage(Actor *attacker, Actor *victim, u16 damage, u8 kind,
                                              u8 abilityRow);
// Element tags describe the attack. They do not apply resistance or poison automatically.
HackLibDamageResult skillExpansion_dealElementalDamage(Actor *attacker, Actor *victim, u16 damage,
                                                       u8 kind, u8 abilityRow, u8 elements);
u16 skillExpansion_handleEnemyStatusArmor(Actor *attacker, Actor *victim, u32 *outHitSfx,
                                          u32 rawDamage);
void skillExpansion_ghostTrigger(Actor *ghost, Actor *victim);
void skillExpansion_trackGhost(Actor *ghost);
void skillExpansion_addHp(u8 playerIndex, s16 amount, bool flinchOnDamage);
void skillExpansion_original_player_addHp(u32 playerIndex, s32 delta, u32 flinchOnDamage);

// Per-player state block of the hero-specific page 2 systems. Only the player's own hero uses it.
// Host tests widen it because their pointers are 8 bytes.
#ifndef SKILL_EXPANSION_HERO_STATE_WORDS
#define SKILL_EXPANSION_HERO_STATE_WORDS 22
#endif
extern u32 skillExpansion_heroState[2][SKILL_EXPANSION_HERO_STATE_WORDS];
// Owner of each block as heroId + 1, or 0 when unowned.
extern u8 skillExpansion_heroStateOwner[2];
// Count of accesses for a hero the player is not, saturating at 255. Expected to stay 0.
extern u8 skillExpansion_heroStateMisuse;
void skillExpansion_noteHeroStateMisuse(void);

/**
 * The block for heroId, where currentHeroId is PLAYER(playerIndex).heroId.
 * The block is zeroed whenever its owning hero changes. An access for another hero is counted
 * in skillExpansion_heroStateMisuse and still returns the block.
 */
static inline u32 *skillExpansion_heroStateClaim(u8 playerIndex, u8 heroId, u8 currentHeroId)
{
    u32 *block = skillExpansion_heroState[playerIndex];
    if (heroId != currentHeroId)
    {
        skillExpansion_noteHeroStateMisuse();
    }
    else if (skillExpansion_heroStateOwner[playerIndex] != heroId + 1)
    {
        for (u32 i = 0; i < SKILL_EXPANSION_HERO_STATE_WORDS; i++)
        {
            block[i] = 0;
        }
        skillExpansion_heroStateOwner[playerIndex] = heroId + 1;
    }
    return block;
}

void skillExpansion_eliteReset(u8 playerIndex);
void skillExpansion_eliteCleanup(u8 playerIndex, Actor *actor);
void skillExpansion_wrapStockCallbacks(u32 heroId, u8 playerIndex);

void skillExpansion_aragornFlameOfTheWest(Actor *playerActor);
void skillExpansion_aragornRangersAmbush(Actor *playerActor);
void skillExpansion_aragornTrackersMark(Actor *playerActor);
void skillExpansion_aragornOathOfTheDead(Actor *playerActor);
void skillExpansion_aragornReturnOfTheKing(Actor *playerActor);

bool32 skillExpansion_canCastFlameOfTheWest(u8 playerIndex);
bool32 skillExpansion_canCastRangersAmbush(u8 playerIndex);
bool32 skillExpansion_canCastTrackersMark(u8 playerIndex);

/** Anduril's Flame meter between Aragorn's HP and Spirit bars. */
void skillExpansion_aragornDrawFlameBar(u8 playerIndex);
bool32 skillExpansion_canCastReturnOfTheKing(u8 playerIndex);
