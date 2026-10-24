#pragma once

// The Gauntlet's screen between rooms (MOD_SCENE_ID(GAUNTLET)): boon pick, level-up rewards and
// the results of a finished run. Arg 0 is the first page (enum GauntletPage).

void gauntlet_sceneEnter(void);
void gauntlet_sceneTick(void);
void gauntlet_sceneExit(void);
