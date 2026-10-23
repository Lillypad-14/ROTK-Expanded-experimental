#include "scene/skillTree.h"
#include "variables.h"

#define SkillTreeCursorDownActive ((u8 *)SkillTreeActiveTilePositions + 0x3c)
#define SkillTreeCursorDownPassive ((u8 *)SkillTreeActiveTilePositions + 0x5a)

u8 scene_skillTree_moveCursorDown(void)
{
    u8 tile = 0;
    s32 v;
    u8 *table;

    switch (SCENE_ARG_SKILL_TREE_GRID)
    {
    case SKILL_TREE_GRID_ACTIVE:
        table = SkillTreeCursorDownActive;
        tile = table[SCENE_ARG_SKILL_TREE_CURSOR];
        v = (s8)table[SCENE_ARG_SKILL_TREE_CURSOR];
        if (v < 0)
        {
            tile = ~v;
            SCENE_ARG_SKILL_TREE_GRID = SKILL_TREE_GRID_PASSIVE;
        }
        break;
    case SKILL_TREE_GRID_PASSIVE:
        table = SkillTreeCursorDownPassive;
        tile = table[SCENE_ARG_SKILL_TREE_CURSOR];
        v = (s8)table[SCENE_ARG_SKILL_TREE_CURSOR];
        if (v < 0)
        {
            tile = ~v;
            SCENE_ARG_SKILL_TREE_GRID = SKILL_TREE_GRID_ACTIVE;
        }
        break;
    }
    return tile;
}
