#define scene_artifacts_enter hdr_dummy
#include "actor.h"
#include "font.h"
#include "gfx.h"
#include "scene.h"
#include "scene/artifacts.h"
#include "sprite.h"
#include "text.h"
#include "variables.h"
#undef scene_artifacts_enter

// Need names: ArtifactsBgControls (0x08050544, u32[4]), ArtifactsBg0Asset (0x08b67d0c),
// ArtifactsPaletteCycles (0x0805053c), and functions 0x0800e69c, 0x0800e61c, 0x0800e84c
// (all void, no args).
#define ArtifactsBgControls ((const u32 *)0x08050544)
#define ArtifactsBg0Asset ((const void *)0x08b67d0c)
#define ArtifactsPaletteCycles ((const void *)0x0805053c)
void artifacts_unknown0800e69c(void);
void artifacts_unknown0800e61c(void);
void artifacts_unknown0800e84c(void);

/**
 * Set up the artifacts gallery: four background layers, the title text and the gallery sprites.
 *
 * @romaddress 0x0800e300
 */
void scene_artifacts_enter(void)
{
    u32 textBgControl;

    gfx_initDisplay(0);
    gfx_enableDisplayFlags(0x1000);
    gfx_setupBg(0, ArtifactsBgControls[0]);
    gfx_loadBgAsset(0, ArtifactsBg0Asset, 1, 0, 0, 0);
    gfx_setupBg(1, ArtifactsBgControls[1]);
    gfx_setBgOffset(1, 0);
    gfx_setupBg(2, ArtifactsBgControls[2]);
    gfx_setBgOffset(2, 0);
    textBgControl = ArtifactsBgControls[3];
    gfx_setupBg(3, textBgControl);
    gfx_loadPalette(UiPalette, 0);
    gfx_setBgOffset(3, 0);
    artifacts_unknown0800e69c();
    artifacts_unknown0800e61c();
    font_setBgTarget(textBgControl);
    font_setStyle(FONT_STYLE_11PX_A, 3, 0);
    font_drawString(1, 0x78, 4, text_getString(0x237), 1);
    artifacts_unknown0800e84c();
    gfx_registerPaletteCycles(ArtifactsPaletteCycles);
    actor_tickAll();
    sprite_endFrame();
    gfx_fadeInFromBlack(0x3f);
}

void artifacts_unknown0800e69c(void)
{
}

void artifacts_unknown0800e61c(void)
{
}

void artifacts_unknown0800e84c(void)
{
}
