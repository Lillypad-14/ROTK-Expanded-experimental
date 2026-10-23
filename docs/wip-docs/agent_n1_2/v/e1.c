#include "font.h"
#include "gfx.h"
#include "player.h"
#include "scene/skillTree.h"
#include "text.h"
#include "variables.h"

// Unnamed BG asset at 0x08baf53c (skill point counter panel).
// Needs data.cfg name e.g. SkillTreePointsBgAsset u8[].
extern const u8 SkillTreePointsBgAsset[];
// Scratch-only stand-in so the harness resolves the symbol.
__asm__(".global SkillTreePointsBgAsset\n.set SkillTreePointsBgAsset, 0x08baf53c");

/**
 * Draw the skill point counter panel for a player.
 *
 * @romaddress 0x08042704
 */
void scene_skillTree_drawSkillPoints(u8 playerIndex)
{
    char text[16];

    gfx_loadBgAsset(2, SkillTreePointsBgAsset, 0x2ff, 6, 14, 11);
    font_setStyle(FONT_STYLE_21PX, 3, -1);
    gfx_fillBgRect(3, 14, 11, 5, 4, 0);
    font_drawString(0xdd, 0x82, 0x5e, text_formatInt(PLAYER(playerIndex).skillPoints, text), 1);
}
