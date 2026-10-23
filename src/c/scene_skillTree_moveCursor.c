#include "scene/skillTree.h"
#include "variables.h"

/**
 * Move the skill tree cursor right, switching grids at an edge. Returns the new tile.
 *
 * @romaddress 0x08041b04
 */
u8 scene_skillTree_moveCursorRight(void)
{
    u8 tile = 0;
    s32 v;
    u8 *table;

    switch (SCENE_ARG_SKILL_TREE_GRID)
    {
    case SKILL_TREE_GRID_ACTIVE:
        table = (u8 *)SkillTreeCursorRightActive;
        tile = table[SCENE_ARG_SKILL_TREE_CURSOR];
        v = (s8)table[SCENE_ARG_SKILL_TREE_CURSOR];
        if (v < 0)
        {
            tile = ~v;
            SCENE_ARG_SKILL_TREE_GRID = SKILL_TREE_GRID_PASSIVE;
        }
        break;
    case SKILL_TREE_GRID_PASSIVE:
        table = (u8 *)SkillTreeCursorRightPassive;
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

/**
 * Move the skill tree cursor left, switching grids at an edge. Returns the new tile.
 *
 * @romaddress 0x08041b64
 */
u8 scene_skillTree_moveCursorLeft(void)
{
    u8 tile = 0;
    s32 v;
    u8 *table;

    switch (SCENE_ARG_SKILL_TREE_GRID)
    {
    case SKILL_TREE_GRID_ACTIVE:
        table = (u8 *)SkillTreeCursorLeftActive;
        tile = table[SCENE_ARG_SKILL_TREE_CURSOR];
        v = (s8)table[SCENE_ARG_SKILL_TREE_CURSOR];
        if (v < 0)
        {
            tile = ~v;
            SCENE_ARG_SKILL_TREE_GRID = SKILL_TREE_GRID_PASSIVE;
        }
        break;
    case SKILL_TREE_GRID_PASSIVE:
        table = (u8 *)SkillTreeCursorLeftPassive;
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

/**
 * Move the skill tree cursor down, switching grids at an edge. Returns the new tile.
 *
 * @romaddress 0x08041bc4
 */
u8 scene_skillTree_moveCursorDown(void)
{
    u8 tile = 0;
    s32 v;
    u8 *table;

    switch (SCENE_ARG_SKILL_TREE_GRID)
    {
    case SKILL_TREE_GRID_ACTIVE:
        table = (u8 *)SkillTreeCursorDownActive;
        tile = table[SCENE_ARG_SKILL_TREE_CURSOR];
        v = (s8)table[SCENE_ARG_SKILL_TREE_CURSOR];
        if (v < 0)
        {
            tile = ~v;
            SCENE_ARG_SKILL_TREE_GRID = SKILL_TREE_GRID_PASSIVE;
        }
        break;
    case SKILL_TREE_GRID_PASSIVE:
        table = (u8 *)SkillTreeCursorDownPassive;
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

/**
 * Move the skill tree cursor up, switching grids at an edge. Returns the new tile.
 *
 * @romaddress 0x08041c24
 */
u8 scene_skillTree_moveCursorUp(void)
{
    u8 tile = 0;
    s32 v;
    u8 *table;

    switch (SCENE_ARG_SKILL_TREE_GRID)
    {
    case SKILL_TREE_GRID_ACTIVE:
        table = (u8 *)SkillTreeCursorUpActive;
        tile = table[SCENE_ARG_SKILL_TREE_CURSOR];
        v = (s8)table[SCENE_ARG_SKILL_TREE_CURSOR];
        if (v < 0)
        {
            tile = ~v;
            SCENE_ARG_SKILL_TREE_GRID = SKILL_TREE_GRID_PASSIVE;
        }
        break;
    case SKILL_TREE_GRID_PASSIVE:
        table = (u8 *)SkillTreeCursorUpPassive;
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
