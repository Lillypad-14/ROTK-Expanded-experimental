#include "patches/debug_cheats/debug_lab.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void *memory_memset(void *destination, u32 value, size_t size)
{
    return memset(destination, value, size);
}

int main(void)
{
    HackLibCombatEvent event = {0};
    debugLab_resetCombatStats();
    event.result = hackLib_damageResult(5, -10, TRUE);
    event.source.kind = HACKLIB_DAMAGE_SUMMON;
    debugLab_recordCombatEvent(0, &event);
    event.result = hackLib_damageResult(20, 20, TRUE);
    debugLab_recordCombatEvent(0, &event);
    event.result = hackLib_damageResult(20, 20, FALSE);
    debugLab_recordCombatEvent(0, &event);
    const DebugLabCombatStats *stats = debugLab_getCombatStats(0);
    assert(stats->attempts == 3 && stats->hits == 2 && stats->kills == 1 && stats->damage == 5);
    event.result = hackLib_damageResult(20, 13, TRUE);
    debugLab_recordCombatEvent(1, &event);
    assert(debugLab_getCombatStats(1)->damage == 7 && stats->damage == 5);
    debugLab_recordCombatEvent(2, &event);
    debugLab_recordCombatEvent(0, NULL);
    assert(stats->attempts == 3 && debugLab_getCombatStats(2) == NULL);
    debugLab_resetCombatStats();
    assert(stats->attempts == 0 && stats->hits == 0 && stats->kills == 0 && stats->damage == 0);
    assert(debugLab_getCombatStats(1)->attempts == 0);
    puts("PASS: combat telemetry counts, overkill, misses, zero-damage hits and reset");
    return 0;
}
