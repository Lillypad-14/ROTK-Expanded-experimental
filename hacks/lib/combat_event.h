#pragma once

#include "lib/combat_result.h"
#include "lib/combat_source.h"

typedef struct HackLibCombatEvent
{
    Actor *attacker;
    Actor *victim;
    HackLibDamageSource source;
    HackLibDamageResult result;
    bool melee;
    // Native melee crit result, when the damage calculation was observed.
    bool criticalKnown;
    // Only meaningful when criticalKnown is TRUE.
    bool critical;
    // Native elemental statuses observed after their application step.
    u8 appliedStatusElements;
    bool appliedStatusElementsKnown;
} HackLibCombatEvent;

typedef void (*HackLibCombatHandler)(const HackLibCombatEvent *event, void *context);

typedef struct HackLibCombatHandlers
{
    HackLibCombatHandler kill;
    HackLibCombatHandler hit;
    HackLibCombatHandler resolved;
} HackLibCombatHandlers;

// Capture immediately after native resolution. Actor pointers are synchronous only.
HackLibCombatEvent hackLib_captureCombatEvent(Actor *attacker, Actor *victim,
                                              HackLibDamageResult result, bool melee);
// Dispatch once per resolution. Kill precedes hit, then resolved also receives misses.
// Nested callbacks restore their parent's source and stop at HACKLIB_MAX_DAMAGE_DEPTH.
bool hackLib_dispatchCombatEvent(const HackLibCombatEvent *event,
                                 const HackLibCombatHandlers *handlers, void *context);
