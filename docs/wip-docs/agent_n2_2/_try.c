#define skill_active_getSpiritCost hdr_dummy
#include "font.h"
#include "gfx.h"
#include "player.h"
#include "scene/skillTree.h"
#include "skill.h"
#include "text.h"
#include "variables.h"
#undef skill_active_getSpiritCost

u32 skill_active_getSpiritCost(u8 playerIndex, u8 activeSkillIndex);

/**
 * Draw the description of the skill under the cursor.
 *
 * @romaddress 0x080421f8
 */
void scene_skillTree_drawDescription(void)
{
    u32 playerIndex = ACTIVE_PLAYER_INDEX;
    u16 passiveBase = TEXT_ID_PASSIVE_SKILL_DESCRIPTION_BASE;
    HeroId heroId = PLAYER(playerIndex).heroId;
    char *text;
    s32 level;

    font_setStyle(FONT_STYLE_8PX_A, 3, -1);
    gfx_fillBgRect(3, 2, 16, 26, 4, 0);
    if (SCENE_ARG_SKILL_TREE_GRID == SKILL_TREE_GRID_ACTIVE)
    {
        u16 baseTextId = ActiveSkillDescriptionBaseTextIds[heroId];

        level = scene_skillTree_getRequiredLevel(playerIndex, SCENE_ARG_SKILL_TREE_CURSOR);
        if (level != -1)
        {
            font_setIntTemplateArg(level, 0);
        }
        else
        {
            font_setTemplateArg(text_getString(TEXT_ID_SKILL_TREE_MAXED), 0);
        }
        font_setIntTemplateArg(skill_active_getSpiritCost(playerIndex, SCENE_ARG_SKILL_TREE_CURSOR), 1);
        text = text_getString(baseTextId + SCENE_ARG_SKILL_TREE_CURSOR);
    }
    else
    {
        u8 skillId = g_SkillTreePassiveTileSkillIds[SCENE_ARG_SKILL_TREE_CURSOR];

        level = scene_skillTree_getRequiredLevel(playerIndex, skillId);
        if (level != -1)
        {
            font_setIntTemplateArg(level, 0);
        }
        else
        {
            font_setTemplateArg(text_getString(TEXT_ID_SKILL_TREE_MAXED), 0);
        }
        text = text_getString(passiveBase + skillId
);
    }
    font_drawLines(0x3d, 16, 0x83, 0xd0, text, 0);
}
