#pragma once

#include "lib/combat_event.h"
#include "patches/mod_options/mod_options.h"

void debugLab_playerTick(u8 playerIndex);
void debugLab_drawTelemetry(u8 playerIndex);
typedef struct DebugLabCombatStats
{
    u32 attempts;
    u32 hits;
    u32 kills;
    u32 damage;
    u16 lastDamage;
    u8 lastKind;
} DebugLabCombatStats;

void debugLab_recordCombatEvent(u8 playerIndex, const HackLibCombatEvent *event);
const DebugLabCombatStats *debugLab_getCombatStats(u8 playerIndex);
void debugLab_resetCombatStats(void);
u32 debugLab_prepareEncounter(u8 playerIndex, u8 kind);
Actor *debugLab_getTarget(u8 index);
void debugLab_clearTargets(void);

#define DEBUG_LAB_OPTION_COUNT 6
extern const struct ModOptionEntry DebugLabOptions[DEBUG_LAB_OPTION_COUNT];
