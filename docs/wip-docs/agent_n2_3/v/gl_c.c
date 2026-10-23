#define scene_griptoniteLogo_tick hdr_dummy
#include "game.h"
#include "gfx.h"
#include "scene.h"
#include "scene/griptoniteLogo.h"
#include "sfx.h"
#include "variables.h"
#undef scene_griptoniteLogo_tick

// Need name: GriptoniteLogoPaletteCycles (0x08051158).
#define GriptoniteLogoPaletteCycles ((const void *)0x08051158)

/**
 * Per-frame state machine of the Griptonite logo: fade in, hold with sparkle, fade out, then
 * hand over to the credits scene.
 *
 * @romaddress 0x0801a7b0
 */
void scene_griptoniteLogo_tick(void)
{
    u32 timer;
    u32 next;

    switch (g_SceneCurrent.args[3])
    {
    case 0:
        timer = g_SceneCurrent.args[4];
        g_SceneCurrent.args[4]--;
        if (timer != 0)
        {
            gfx_fadeSetDarken(1, g_SceneCurrent.args[4]);
        }
        else
        {
            g_SceneCurrent.args[3] = 1;
            g_SceneCurrent.args[4] = 0x50;
            gfx_registerPaletteCycles(GriptoniteLogoPaletteCycles);
            gfx_fadeSetDarken(2, 0x10);
            gfx_setBlendAlpha(0, 0x10);
            gfx_setBlendControl(2, 0x21);
            gfx_unknown08012444(1);
        }
        break;
    case 1:
        if (g_SceneCurrent.args[4] != 0)
        {
            if (g_SceneCurrent.args[4] == 0x4b)
            {
                sfx_play(6);
            }
            if (g_SceneCurrent.args[4] < 0x1f && (g_SceneCurrent.args[4] & 1) == 0)
            {
                gfx_setBlendAlpha(0x10 - (g_SceneCurrent.args[4] >> 1), g_SceneCurrent.args[4] >> 1);
            }
            g_SceneCurrent.args[4]--;
        }
        else
        {
            gfx_fadeSetDarken(2, 0);
            gfx_fadeSetDarken(1, 0x10);
            gfx_disableBg(0);
            gfx_clearPaletteAnims();
            g_SceneCurrent.args[3] = 2;
            g_SceneCurrent.args[4] = 0x3c;
        }
        break;
    case 2:
        timer = g_SceneCurrent.args[4];
        g_SceneCurrent.args[4]--;
        if (timer == 0)
        {
            g_SceneCurrent.args[3] = 4;
        }
        break;
    case 4:
        game_requestSceneChange(SCENE_ID_CREDITS);
        break;
    }
}
