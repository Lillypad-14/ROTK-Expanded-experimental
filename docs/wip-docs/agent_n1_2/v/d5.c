#include "font.h"
#include "gfx.h"
#include "player.h"
#include "scene/skillTree.h"
#include "text.h"
#include "variables.h"
extern const u8 SkillTreePointsBgAsset[];
__asm__(".global SkillTreePointsBgAsset\n.set SkillTreePointsBgAsset, 0x08baf53c");
void scene_skillTree_drawSkillPoints(u32 playerIndex)
{
    char text[16];
    Player *player = &PLAYER((u8)playerIndex);
    gfx_loadBgAsset(2, SkillTreePointsBgAsset, 0x2ff, 6, 14, 11);
    font_setStyle(FONT_STYLE_21PX, 3, -1);
    gfx_fillBgRect(3, 14, 11, 5, 4, 0);
    font_drawString(0xdd, 0x82, 0x5e, text_formatInt(player->skillPoints, text), 1);
}
