#include "patches/experimental/experimental.h"

#include "game.h"
#include "system.h"
#include "variables.h"

// Vblanks per game tick, set to 2 by rotk_main before its loop.
#define TICK_INTERVAL (*(volatile u32 *)0x03004140)

static u32 vblanksSinceTick(void)
{
    s32 elapsed = (s32)(g_EngineState->vblankCounter - g_EngineState->lastTickVblank);

    return elapsed < 0 ? -elapsed : elapsed;
}

/**
 * The rotk_main loop. The stock loop always waits for at least one vblank after a tick, so a
 * tick that runs slightly long idles until the next one. With frame pacing on, a late tick is
 * followed at once by the next, whose logic overlaps the wait for the previous frame's commit.
 */
void framePacing_mainLoop(void)
{
    for (;;)
    {
        game_tickFrame();
        if (!g_ExperimentalState.framePacing)
        {
            system_waitVBlank();
        }
        while (vblanksSinceTick() < TICK_INTERVAL)
        {
            system_waitVBlank();
        }
        g_EngineState->lastTickVblank = g_EngineState->vblankCounter;
    }
}

void framePacing_awaitCommit(void)
{
    if (!g_ExperimentalState.framePacing)
    {
        return;
    }
    while (g_EngineState->frameReady != 0)
    {
        system_waitVBlank();
    }
}
