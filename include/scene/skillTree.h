#pragma once

#include "types.h"

// Scene 0x0f: skill tree.
void scene_skillTree_enter(void);
void scene_skillTree_tick(void);
void scene_skillTree_exit(void);

/** SceneState.args scratch-slot roles for the skill tree. */
enum SkillTreeArg
{
    SCENE_ARG_SKILL_TREE_CURSOR_INDEX = 1,
    SCENE_ARG_SKILL_TREE_HAS_SKILL_POINTS_INDEX = 2,
    SCENE_ARG_SKILL_TREE_GRID_INDEX = 3,
};

// g_SceneCurrent.args accessors for the slots above.
#define SCENE_ARG_SKILL_TREE_CURSOR g_SceneCurrent.args[SCENE_ARG_SKILL_TREE_CURSOR_INDEX]
#define SCENE_ARG_SKILL_TREE_HAS_SKILL_POINTS                                                      \
    g_SceneCurrent.args[SCENE_ARG_SKILL_TREE_HAS_SKILL_POINTS_INDEX]
#define SCENE_ARG_SKILL_TREE_GRID g_SceneCurrent.args[SCENE_ARG_SKILL_TREE_GRID_INDEX]

/** SCENE_ARG_SKILL_TREE_GRID values: the grid the cursor is on. */
enum SkillTreeGrid
{
    SKILL_TREE_GRID_ACTIVE = 1,
    SKILL_TREE_GRID_PASSIVE = 2,
};

// Tiles per grid. Active tile n shows active skill row n.
#define SKILL_TREE_ACTIVE_TILE_COUNT 6
#define SKILL_TREE_PASSIVE_TILE_COUNT 9

enum SkillTreeTextId
{
    // Template arg 0 of a description when the skill is maxed.
    TEXT_ID_SKILL_TREE_MAXED = 0x1e2,

    // Plus PassiveSkillId: a passive skill's description.
    TEXT_ID_PASSIVE_SKILL_DESCRIPTION_BASE = 0x1e3,
};

// Passive tiles are ordered by required level up to this level.
#define SKILL_TREE_MAX_REQUIRED_LEVEL 20

/** Move the cursor with the d-pad, crossing between the grids at their inner edges. */
void scene_skillTree_handleDpad(void);

/**
 * Cursor tile after pressing right, left, down or up. Right and left can switch
 * SCENE_ARG_SKILL_TREE_GRID and return a tile on the other grid.
 */
u8 scene_skillTree_moveCursorRight(void);
u8 scene_skillTree_moveCursorLeft(void);
u8 scene_skillTree_moveCursorDown(void);
u8 scene_skillTree_moveCursorUp(void);
void scene_skillTree_createCursor(void);
void scene_skillTree_unknown0804269c(s32 playerIndex, s32 row);

/** Draw a passive tile: hero background, skill icon and level, or the learned check on tile 8. */
void scene_skillTree_drawPassiveTile(u8 playerIndex, u8 passiveSkillId, u8 tile, u8 level);

/** Draw an active tile, greyed out when the row is out of the cast cycle. */
void scene_skillTree_drawActiveTile(u8 playerIndex, u8 activeSkillIndex, u8 tile, u8 level);

/** Spend a skill point on the skill under the cursor. */
void scene_skillTree_buySkill(u8 playerIndex);
u8 scene_skillTree_drawActiveTiles(u8 playerIndex, u32 startTile);
void scene_skillTree_unknown08042674(void);
void scene_skillTree_unknown08042678(void);
/** Take one skill point from the player after a purchase. */
void scene_skillTree_spendSkillPoint(u8 playerIndex);

/** A-button handler: buy the skill under the cursor for the active player. */
void scene_skillTree_handleA(void);

/** Draw the description of the skill under the cursor. */
void scene_skillTree_drawDescription(void);

/**
 * Player level the next purchase needs, or -1 when the skill is maxed.
 *
 * @param skill an active skill row on the active grid, a PassiveSkillId on the passive grid
 */
s32 scene_skillTree_getRequiredLevel(s32 playerIndex, s32 skill);

/** Bind the active skill under the cursor to the next quick-select slot. */
void scene_skillTree_assignQuickSelect(void);

void scene_skillTree_drawSkillPoints(u8 playerIndex);

/** Fill g_SkillTreePassiveTileSkillIds in required-level order and draw the passive tiles. */
void scene_skillTree_drawPassiveGrid(u8 playerIndex);

/**
 * Draw the passive slots whose first level requires `level`, from `tile` on.
 *
 * @return the next free tile
 */
u8 scene_skillTree_drawPassiveTilesAtLevel(u8 playerIndex, u8 tile, u8 level);

/** Draw the active tiles for rows 0..5. */
void scene_skillTree_drawActiveGrid(u8 playerIndex);
