#include "mission_scripts.h"

u8 hackLib_suppressMissionScripts;

// The original bodies past their 8-byte entry hooks.
asm(".section .text\n"
    ".balign 4\n.thumb\n.global hackLib_originalSpawnGroup\n.thumb_func\n"
    "hackLib_originalSpawnGroup:\n"
    "push {r4-r7,lr}\nmov r7, r8\npush {r7}\nlsl r0, r0, #24\n"
    "ldr r3, =mission_script_spawnGroup + 8\nbx r3\n.pool\n"
    ".balign 4\n.global hackLib_originalRunStream\n.thumb_func\n"
    "hackLib_originalRunStream:\n"
    "push {r4-r7,lr}\nlsl r0, r0, #24\nlsr r0, r0, #24\nldr r1, =g_MissionCurrentStream\n"
    "ldr r3, =mission_script_runStream + 8\nbx r3\n.pool\n");

void hackLib_originalSpawnGroup(u8 groupIndex);
void hackLib_originalRunStream(u8 streamIndex);

void hackLib_missionSpawnGroup(u8 groupIndex)
{
    if (!hackLib_suppressMissionScripts)
    {
        hackLib_originalSpawnGroup(groupIndex);
    }
}

void hackLib_missionRunStream(u8 streamIndex)
{
    if (!hackLib_suppressMissionScripts)
    {
        hackLib_originalRunStream(streamIndex);
    }
}
