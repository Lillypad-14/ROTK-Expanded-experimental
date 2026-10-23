#include "font.h"
#include "gfx.h"
#include "player.h"
#include "scene/skillTree.h"
#include "text.h"
#include "variables.h"

#define SkillTreePointsBgAsset ((void *)0x08baf53c) // TODO: name in data.cfg

/**
 * Draw the skill-point counter box of a player.
 *
 * @romaddress 0x08042704
 */
void scene_skillTree_drawSkillPoints(u8 playerIndex)
{
    char buf[16];
    Player *players = g_PlayerGlobals.players;
    Player *player = &players[playerIndex];

    gfx_loadBgAsset(2, SkillTreePointsBgAsset, 0x2ff, 6, 0xe, 0xb);
    font_setStyle(FONT_STYLE_21PX, 3, -1);
    gfx_fillBgRect(3, 0xe, 0xb, 5, 4, 0);
    font_drawString(0xdd, 0x82, 0x5e, text_formatInt(player->skillPoints, buf), 1);
}
