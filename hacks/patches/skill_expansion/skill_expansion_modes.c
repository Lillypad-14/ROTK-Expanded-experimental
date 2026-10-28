// Hero kits: a character of one of the 8 stock heroes plays with the Expanded kit (new skills, page
// 2, its own mechanics) or the vanilla one, chosen with L on the hero select when the character is
// created (new game, bonus maps, co-op). The choice belongs to the save slot: bit 0 of its skill
// record's reservedHeader[0] (skill_expansion_save.c), so existing saves, which hold 0 there, stay
// Expanded. In co-op each player's own character keeps its kit (the page 2 record carries it).
// Saruman and the Ringwraith exist only in Expanded and always play their kits.
#include "patches/skill_expansion/skill_expansion.h"
#include "player.h"
#include "scene.h"
#include "scene/saveLoadMenu.h"
#include "sfx.h"
#include "input.h"
#include "font.h"
#include "gfx.h"

bool32 skillExpansion_playerExpanded(u8 playerIndex)
{
    return playerIndex >= 2 || PLAYER(playerIndex).heroId >= 8 ||
           !skillExpansion_players[playerIndex].vanillaKit;
}

bool32 skillExpansion_heroSelectVanilla(void)
{
    return SCENE_ARG_SAVE_LOAD_CURSOR < 8 && skillExpansion_players[ACTIVE_PLAYER_INDEX].newHeroVanilla;
}

bool32 skillExpansion_heroSelectTick(void)
{
    SkillExpansionPlayer *sx = &skillExpansion_players[ACTIVE_PLAYER_INDEX];
    u32 keys = ACTIVE_MENU_KEYS_PRESSED;

    if (SCENE_ARG_SAVE_LOAD_STATE != SAVE_LOAD_STATE_HERO_SELECT)
    {
        return FALSE;
    }
    if ((keys & L_BUTTON) && SCENE_ARG_SAVE_LOAD_CURSOR < 8)
    {
        sx->newHeroVanilla = !sx->newHeroVanilla;
        scene_saveLoadMenu_drawSlot(SCENE_ARG_SAVE_LOAD_SELECTED_SLOT, SCENE_ARG_SAVE_LOAD_SELECTED_SLOT);
        sfx_play(SFX_MENU_CURSOR_MOVE);
        return TRUE;
    }
    if (keys & A_BUTTON)
    {
        // The stock confirm creates the character next, building its loadout with this kit.
        sx->vanillaKit = skillExpansion_heroSelectVanilla();
    }
    return FALSE;
}

// New game hero select (scene 0x08, scene_newGame_tick): its own screen, the cursor's hero in args[1]
// and g_NewGameState (0x03001ab4) nonzero while a hero can be picked. args[4], unused by the stock
// scene and zeroed on entry, remembers the label drawn: 1 + hero * 2 + vanilla.
#define NEW_GAME_STATE (*(volatile u8 *)0x03001ab4)
#define NEW_GAME_HERO g_SceneCurrent.args[1]
#define NEW_GAME_LABEL g_SceneCurrent.args[4]
// The label: BG0 (the scene's text layer) glyph tiles past the hero name's, top right of the title.
#define NEW_GAME_LABEL_TILE 0xf0
#define NEW_GAME_LABEL_X 0xa8
#define NEW_GAME_LABEL_Y 4

static void drawNewGameLabel(void)
{
    SkillExpansionPlayer *sx = &skillExpansion_players[0];
    u32 label = NEW_GAME_HERO < 8 ? 1 + NEW_GAME_HERO * 2 + (sx->newHeroVanilla ? 1 : 0) : 0;

    if (label == NEW_GAME_LABEL)
    {
        return;
    }
    NEW_GAME_LABEL = label;
    gfx_fillBgRect(0, NEW_GAME_LABEL_X / 8, 0, 30 - NEW_GAME_LABEL_X / 8, 2, 0);
    if (label != 0)
    {
        font_setStyle(FONT_STYLE_8PX_A, 3, -1);
        font_drawString(NEW_GAME_LABEL_TILE, NEW_GAME_LABEL_X, NEW_GAME_LABEL_Y,
                        sx->newHeroVanilla ? "L: Vanilla kit" : "L: Expanded kit", 0);
    }
}

bool32 skillExpansion_newGameTick(void)
{
    SkillExpansionPlayer *sx = &skillExpansion_players[0];
    u32 keys = PLAYER_KEYS_PRESSED(ACTIVE_PLAYER_INDEX);

    if (NEW_GAME_STATE == 0)
    {
        return FALSE;
    }
    if ((keys & L_BUTTON) && NEW_GAME_HERO < 8)
    {
        sx->newHeroVanilla = !sx->newHeroVanilla;
        drawNewGameLabel();
        sfx_play(SFX_MENU_CURSOR_MOVE);
        return TRUE;
    }
    if (keys & (A_BUTTON | START_BUTTON))
    {
        // The stock confirm builds the new character's loadout next, with this kit.
        sx->vanillaKit = NEW_GAME_HERO < 8 && sx->newHeroVanilla;
    }
    drawNewGameLabel();
    return FALSE;
}
