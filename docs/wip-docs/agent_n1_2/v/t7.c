#include "game.h"
#include "gfx.h"
#include "scene.h"
#include "variables.h"

// TransitionState.field_0x09[] roles: [0] frame timer (s8), [1] frame reload, [2] frame count,
// [3] current frame (s8). field_0x00 is a pointer to the frame asset table.
#define TRANSITION_TIMER field_0x09[0]
#define TRANSITION_RELOAD field_0x09[1]
#define TRANSITION_COUNT field_0x09[2]
#define TRANSITION_FRAME field_0x09[3]

/**
 * Advance the active transition effect one vblank: step to the next backdrop frame when the
 * frame timer runs out.
 *
 * @romaddress 0x0804a584
 */
void transition_tick(void)
{
    if (g_GameFlags.p & (GAME_FLAG_30 | GAME_FLAG_20 | GAME_FLAG_REQUEST_OVERLAY | GAME_FLAG_18 |
                         GAME_FLAG_SCENE_CHANGE | GAME_FLAG_WORLD_PARKED)) {
        return;
    }
    if (!(g_TransitionState.flags & TRANSITION_FLAG_SFX_ACTIVE)) {
        return;
    }
    if ((u8)(g_TransitionState.flags & 4)) {
        return;
    }
    if ((s8)g_TransitionState.TRANSITION_TIMER <= 0) {
        g_TransitionState.TRANSITION_FRAME++;
        if ((s8)g_TransitionState.TRANSITION_FRAME >= g_TransitionState.TRANSITION_COUNT) {
            g_TransitionState.TRANSITION_FRAME = 0;
        }
        g_TransitionState.TRANSITION_TIMER = g_TransitionState.TRANSITION_RELOAD;
        if (g_TransitionState.field_0x00 != 0 && g_TransitionState.effect != 0) {
            { s8 f = g_TransitionState.TRANSITION_FRAME; void **t = (void**)g_TransitionState.field_0x00; gfx_unknown08011bc0(1, t[f], 0x9f, 2); }
        }
    }
    g_TransitionState.TRANSITION_TIMER--;
}
