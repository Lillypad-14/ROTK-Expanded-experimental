#include "memory.h"
#include "patches/debug_cheats/debug_lab.h"

static DebugLabCombatStats combatStats[2];

void debugLab_recordCombatEvent(u8 playerIndex, const HackLibCombatEvent *event)
{
    if (playerIndex >= ARRAY_COUNT(combatStats) || event == NULL)
    {
        return;
    }
    DebugLabCombatStats *stats = &combatStats[playerIndex];
    stats->attempts++;
    stats->hits += event->result.hit;
    stats->kills += event->result.hit && event->result.killed;
    if (event->result.hit && event->result.damageDealt > 0)
    {
        stats->damage += event->result.damageDealt;
    }
    stats->lastDamage = event->result.damageDealt;
    stats->lastKind = event->source.kind;
}

const DebugLabCombatStats *debugLab_getCombatStats(u8 playerIndex)
{
    return playerIndex < ARRAY_COUNT(combatStats) ? &combatStats[playerIndex] : NULL;
}

void debugLab_resetCombatStats(void)
{
    memory_memset(combatStats, 0, sizeof(combatStats));
}
