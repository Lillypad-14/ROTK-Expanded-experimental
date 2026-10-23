#include "gba_io.h"
#include "input.h"
#include "player.h"
#include "scene/skillTree.h"
#include "sfx.h"
#include "sprite.h"
#include "variables.h"

/**
 * Move the cursor with the d-pad, crossing between the grids at their inner edges.
 *
 * @romaddress 0x08041a24
 */
void scene_skillTree_handleDpad(void)
{
    u8 oldCursor = SCENE_ARG_SKILL_TREE_CURSOR;
    u32 cursor = oldCursor;
    u8 playerIndex = ACTIVE_PLAYER_INDEX;
    u8 oldGrid = SCENE_ARG_SKILL_TREE_GRID;

    if (SCENE_ARG_SKILL_TREE_GRID - 1 < 2)
    {
        switch (MENU_KEYS_PRESSED(playerIndex) & DPAD_ANY)
        {
        case DPAD_RIGHT:
            cursor = scene_skillTree_moveCursorRight();
            break;
        case DPAD_LEFT:
            cursor = scene_skillTree_moveCursorLeft();
            break;
        case DPAD_UP:
            cursor = scene_skillTree_moveCursorUp();
            break;
        case DPAD_DOWN:
            cursor = scene_skillTree_moveCursorDown();
            break;
        }
        SCENE_ARG_SKILL_TREE_CURSOR = cursor;
        if (oldCursor != cursor || oldGrid != SCENE_ARG_SKILL_TREE_GRID)
        {
            scene_skillTree_drawDescription();
            if (SCENE_ARG_SKILL_TREE_GRID != SKILL_TREE_GRID_ACTIVE)
            {
                if (SCENE_ARG_SKILL_TREE_GRID == SKILL_TREE_GRID_PASSIVE)
                {
                    sprite_setPosition(g_SkillTreeCursorObj,
                                       SkillTreePassiveTilePositions[SCENE_ARG_SKILL_TREE_CURSOR].x << 19,
                                       SkillTreePassiveTilePositions[SCENE_ARG_SKILL_TREE_CURSOR].y << 19);
                }
            }
            else
            {
                sprite_setPosition(g_SkillTreeCursorObj,
                                   SkillTreeActiveTilePositions[SCENE_ARG_SKILL_TREE_CURSOR].x << 19,
                                   SkillTreeActiveTilePositions[SCENE_ARG_SKILL_TREE_CURSOR].y << 19);
            }
            sfx_play(2);
        }
    }
}
