#pragma once

#include "types.h"

/** TRUE turns story spawn groups and scripts off, so a mission's terrain plays empty. */
extern u8 hackLib_suppressMissionScripts;

/** REPLACES (whole function, when test_arena or gauntlet is enabled): mission_script_spawnGroup. */
void hackLib_missionSpawnGroup(u8 groupIndex);

/** REPLACES (whole function, when test_arena or gauntlet is enabled): mission_script_runStream. */
void hackLib_missionRunStream(u8 streamIndex);
