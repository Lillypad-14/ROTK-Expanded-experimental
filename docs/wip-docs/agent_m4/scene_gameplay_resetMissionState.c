#include "memory.h"
#include "scene.h"
#include "variables.h"

extern u32 g_UnknownMissionState_03004164;
extern u32 g_UnknownMissionState_03004170;

/**
 * Reset the per-mission state for a fresh gameplay entry.
 *
 * @romaddress 0x0803238c
 */
void scene_gameplay_resetMissionState(void)
{
    g_SceneCurrent.args[0] = 0;
    g_ObjPaletteSnapshot = memory_zalloc(512);
    g_UnknownMissionState_03004164 = 0;
    g_CurrentMissionId = 130;
    g_UnknownMissionState_03004170 = 130;
}
