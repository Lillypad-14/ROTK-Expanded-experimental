#pragma once

// Gandalf's page 2: The Grey and the White. Holding R swaps between Gandalf the Grey, who saves
// Spirit, and Gandalf the White, who spends it. Each page 2 row casts its Grey or White half by the
// current form. Every entry point does nothing for other heroes.

#include "actor.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "types.h"

void skillExpansion_gandalfReset(u8 playerIndex);
void skillExpansion_gandalfCleanup(u8 playerIndex, Actor *actor);
void skillExpansion_gandalfTick(u8 playerIndex, Actor *actor);

/** Reads the hold-R form swap and cancels Pipe-weed Pause on any input. */
void skillExpansion_gandalfInput(u8 playerIndex);

/** TRUE while the active player's Gandalf is the White. */
bool32 skillExpansion_gandalfIsWhite(void);

/** Damage of a hit Gandalf deals, with the White's spell bonus and his zones. */
s32 skillExpansion_gandalfDamage(u8 playerIndex, Actor *victim, s32 damage, bool melee);

/** Damage Gandalf takes. The Fall, Robes of White and Breaking of the Staff act here. */
s32 skillExpansion_gandalfIncomingDamage(u8 playerIndex, Actor *origin, s32 damage);

void skillExpansion_gandalfOnKill(u8 playerIndex, Actor *victim);

SkillExpansionCastResult skillExpansion_gandalfFireworks(Actor *playerActor);
SkillExpansionCastResult skillExpansion_gandalfNeverLate(Actor *playerActor);
SkillExpansionCastResult skillExpansion_gandalfSmokeRings(Actor *playerActor);
SkillExpansionCastResult skillExpansion_gandalfPipeWeed(Actor *playerActor);
SkillExpansionCastResult skillExpansion_gandalfFlyYouFools(Actor *playerActor);
SkillExpansionCastResult skillExpansion_gandalfShallNotPass(Actor *playerActor);

/** FALSE while A Wizard Is Never Late's mark waits. */
bool32 skillExpansion_canCastGandalfNeverLate(u8 playerIndex);

/** FALSE while Gandalf smokes his pipe. */
bool32 skillExpansion_canCastGandalfPipeWeed(u8 playerIndex);

/** FALSE while You Shall Not Pass! holds the line. */
bool32 skillExpansion_canCastGandalfSkill(u8 playerIndex);
