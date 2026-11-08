#pragma once

#include "actor.h"

typedef struct HackLibActorRef
{
    Actor *actor;
    u32 generation;
} HackLibActorRef;

void hackLib_noteActorAllocated(Actor *actor);
void hackLib_noteActorRetired(Actor *actor);
HackLibActorRef hackLib_actorRef(Actor *actor);
bool hackLib_isActorRefValid(HackLibActorRef reference);
s32 hackLib_actorCurrentHp(const Actor *actor);
/** The actor's max HP: a player's from its stats, an enemy's from the enemy table, else 0. */
s32 hackLib_actorMaxHp(const Actor *actor);
u32 hackLib_actorGeneration(Actor *actor);

enum HackLibProbeKind
{
    HACKLIB_PROBE_EFFECT, // a timed effect (status, debuff) put on the player
    HACKLIB_PROBE_STUN,   // a stun put on the player
    HACKLIB_PROBE_HIT,    // a skill or player hit that landed on the player
    HACKLIB_PROBE_COUNT
};

/**
 * Per player wrapping counts of what other players' skills did to them (enum HackLibProbeKind),
 * read from outside by the PC co-op test trace.
 */
extern u8 g_HackLibPlayerProbe[HACKLIB_PROBE_COUNT][2];

/**
 * Run the code that follows as the player behind `actor` (a player avatar or an actor it owns), so a
 * duel knows whose effect it is. Returns the previous state for hackLib_endActing. Without the
 * versus patch these do nothing.
 */
u8 hackLib_beginActingFor(Actor *actor);
void hackLib_endActing(u8 previous);

/** Count `kind` for the player whose avatar is `target`, if it is one. */
void hackLib_probePlayer(u32 kind, const Actor *target);
