#include "actor.h"
#include "scene.h"
#include "scene/skillTree.h"
#include "sprite.h"
#include "variables.h"

// Stand-in for the cursor animation at 0x0806e3c4.
#define SkillTreeCursorAnimation ((const SpriteAnimation *)0x0806e3c4)

/**
 * Create the cursor actor on the tile under the cursor and draw its description.
 *
 * @romaddress 0x080423e8
 */
void scene_skillTree_createCursor(void)
{
    g_SkillTreeCursorObj = actor_allocOverlay();
    g_SkillTreeCursorObj->type = 1;
    sprite_setPosition(g_SkillTreeCursorObj, SkillTreePassiveTilePositions[SCENE_ARG_SKILL_TREE_CURSOR].x << 19,
                       SkillTreePassiveTilePositions[SCENE_ARG_SKILL_TREE_CURSOR].y << 19);
    g_SkillTreeCursorObj->flags.p = ACTOR_FLAG_RENDER;
    g_SkillTreeCursorObj->oam.objMode = 0;
    g_SkillTreeCursorObj->oam.colors256 = 0;
    g_SkillTreeCursorObj->oam.paletteNumber = 3;
    g_SkillTreeCursorObj->oam.bgPriority = 0;
    sprite_setAnimation(g_SkillTreeCursorObj, SkillTreeCursorAnimation);
    scene_skillTree_drawDescription();
}
