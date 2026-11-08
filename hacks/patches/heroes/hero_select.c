#include "patches/heroes/hero_select.h"
#include "font.h"
#include "game.h"
#include "gba_io.h"
#include "gfx.h"
#include "system.h"
#if HACK_fast_fade
#include "patches/fast_fade/fast_fade.h"
#endif
#include "hero_group_count.h"
#include "hero_stat_declarations.h"
#include "input.h"
#include "patches/gauntlet/gauntlet_ui.h"
#include "patches/ringwraith/ringwraith.h"
#include "save.h"
#include "scene.h"
#include "scene/languageSelect.h"
#include "scene/saveLoadMenu.h"
#include "sfx.h"
#include "sprite.h"
#include "variables.h"

#if PC_EDITION && HACK_gauntlet && HACK_ringwraith

#include "mod_scenes.h"
#if HACK_skill_expansion
#include "patches/skill_expansion/skill_expansion.h"
#endif

#include "hero_custom_portraits.inc"

const HeroMenuInfo HeroMenuInfos[HACK_HERO_COUNT] = {
#include "hero_menu.inc"
};

static const char *const HeroGroupNames[HERO_GROUP_COUNT] = {
#include "hero_groups.inc"
};

#define ARG_GROUP g_SceneCurrent.args[0]
// Index into the group's heroes. The page shown is the cursor's.
#define ARG_CURSOR g_SceneCurrent.args[1]

#define BG_ART_BACK 0u
#define BG_TEXT 2u
#define BG_ART_FRONT 3u
#define ART_FRONT_BGCNT (BGCNT_PRIORITY(2) | BGCNT_CHARBASE(1) | BGCNT_SCREENBASE(0x1eu))
#define TEXT_BGCNT GAUNTLET_UI_TEXT_BGCNT

#define COLOR_WHITE 0
#define COLOR_YELLOW 3
#define COLOR_GREEN 9

#define CELL 8
#define CENTER_X 0x78
#define TITLE_Y 8
#define GROUP_Y 24
#define HINT_Y 148

// Cards: 3 x 2 of 4 x 6 cells inside the left panel, a page at a time.
#define GRID_X 1
#define GRID_Y 4
#define GRID_W 14
#define GRID_H 14
#define CARD_COLUMNS 3
#define CARD_ROWS 2
#define CARDS_PER_PAGE (CARD_COLUMNS * CARD_ROWS)
#define CARD_W 4
#define CARD_H 6
#define CARD_X(i) (GRID_X + 1 + ((i) % CARD_COLUMNS) * CARD_W)
#define CARD_Y(i) (GRID_Y + 1 + ((i) / CARD_COLUMNS) * CARD_H)
// Menu sprites stand on this point of their card (their feet), in pixels from its top left.
#define SPRITE_X 16
#define SPRITE_Y 39
// OAM texture step of an enemy-body hero's card sprite (8.8, 0x100 = full size): about 70%.
#define ENEMY_BODY_SCALE 0x170
// The stock heroes' menu sprites at their own size: they are built of many small pieces, and a scale
// turns each piece about its own center, leaving seams through the body.
#define STOCK_BODY_SCALE 0x100
// Names longer than this many letters overflow the panel in the large font.
#define NAME_LARGE_MAX 14
// Frames of the screen's fades in and out.
#define FADE_STEPS 10

// Details panel: portrait with stats and kit beside it, name and description below.
#define SIDE_X 15
#define SIDE_Y 4
#define SIDE_W 14
#define SIDE_H 14
#define SIDE_CENTER_X ((SIDE_X * 2 + SIDE_W) * CELL / 2)
#define PORTRAIT_X (SIDE_X + 1)
#define PORTRAIT_Y (SIDE_Y + 1)
#define STATS_X ((PORTRAIT_X + GAUNTLET_UI_PORTRAIT_W) * CELL + 2)
#define STATS_Y (PORTRAIT_Y * CELL + 2)
#define STAT_LINE 10
#define KIT_Y (STATS_Y + 5 * STAT_LINE + 2)
#define NAME_Y ((PORTRAIT_Y + GAUNTLET_UI_PORTRAIT_H) * CELL + 3)
#define DESCRIPTION_Y (NAME_Y + 13)
#define DESCRIPTION_LINES 2

static Actor *Sprites[CARDS_PER_PAGE];
// Each card sprite's palette, NULL for a hidden card.
static const u16 *CardPalettes[CARDS_PER_PAGE];
// The hero picked here plus 1, waiting for the save menu (0: none). Zero-initialized: modcode has
// no initialized data.
static u8 pendingChoice;
static bool committing;

static bool32 unlocked(u32 heroId)
{
    if (heroId == HERO_ID_SAM)
    {
        return (g_SaveGlobals.d.unlockFlags.p & UNLOCK_FLAG_CHARACTER_SAM) != 0;
    }
    if (heroId == HERO_ID_SMEAGOL)
    {
        return (g_SaveGlobals.d.unlockFlags.p & UNLOCK_FLAG_CHARACTER_SMEAGOL) != 0;
    }
    return TRUE;
}

/** The heroes of the current group, in registry order. Returns how many. */
static u32 groupHeroes(u8 *heroes)
{
    u32 count = 0;

    for (u32 hero = 0; hero < HACK_HERO_COUNT; hero++)
    {
        if (HeroMenuInfos[hero].group == ARG_GROUP)
        {
            heroes[count++] = (u8)hero;
        }
    }
    return count;
}

#if HACK_skill_expansion
/** Stock heroes can be created with their vanilla kit (skill_expansion_modes.c). */
static bool32 hasKitChoice(u32 heroId)
{
    return heroId < 8 && unlocked(heroId);
}

static bool32 vanillaKit(void)
{
    return skillExpansion_players[ACTIVE_PLAYER_INDEX].newHeroVanilla;
}
#endif

static s32 drawText(s32 tile, s32 color, s32 x, s32 y, const char *text, s32 alignment)
{
    font_setStyle(FONT_STYLE_8PX_B, color, GAUNTLET_UI_TEXT_BACKGROUND);
    return font_drawString(tile, x, y, (char *)text, alignment);
}

static char *appendText(char *out, const char *text)
{
    while (*text != '\0')
    {
        *out++ = *text++;
    }
    *out = '\0';
    return out;
}

static char *appendNumber(char *out, u32 value)
{
    char digits[4];
    u32 count = 0;

    do
    {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0 && count < sizeof(digits));
    while (count != 0)
    {
        *out++ = digits[--count];
    }
    *out = '\0';
    return out;
}

static s32 drawStats(s32 tile, const HeroBaseStats *stats)
{
    static const char *const Labels[] = {"Crg ", "Def ", "Hlt ", "Acc ", "Str "};
    const u16 values[] = {stats->courage, stats->defense, stats->health, stats->accuracy,
                          stats->strength};
    char buffer[12];

    for (u32 i = 0; i < ARRAY_COUNT(Labels); i++)
    {
        appendNumber(appendText(buffer, Labels[i]), values[i]);
        tile = drawText(tile, values[i] >= 28 ? COLOR_GREEN : COLOR_WHITE, STATS_X,
                        STATS_Y + i * STAT_LINE, buffer, 0);
    }
    return tile;
}

static void showSprites(const u8 *heroes, u32 count, u32 first)
{
    for (u32 i = 0; i < CARDS_PER_PAGE; i++)
    {
        const SpriteAnimation *animation;
        const u16 *palette;
        u32 index = first + i;

        if (index >= count || !unlocked(heroes[index]) ||
            !ringwraith_menuSprite(heroes[index], index == ARG_CURSOR, &animation, &palette))
        {
            Sprites[i]->flags.p &= ~ACTOR_FLAG_RENDER;
            CardPalettes[i] = NULL;
            continue;
        }
        Sprites[i]->flags.p |= ACTOR_FLAG_RENDER;
        sprite_setAnimation(Sprites[i], animation);
        // Shrink every body to fit its card: enemy bodies are drawn larger than the stock heroes.
        u16 scale = heroes[index] >= 8 ? ENEMY_BODY_SCALE : STOCK_BODY_SCALE;

        sprite_setScale(Sprites[i], scale, scale, 0);
        CardPalettes[i] = palette;
        gfx_queuePaletteWrite(palette, i << 4, 16);
    }
}

static s32 drawDetails(s32 tile, u32 hero)
{
    char *remaining;

    gauntletUi_panel(SIDE_X, SIDE_Y, SIDE_W, SIDE_H);
    if (!unlocked(hero))
    {
        font_setStyle(FONT_STYLE_11PX_A, COLOR_YELLOW, GAUNTLET_UI_TEXT_BACKGROUND);
        tile = font_drawString(tile, SIDE_CENTER_X, NAME_Y, "Locked", 1);
        return drawText(tile, COLOR_WHITE, SIDE_CENTER_X, DESCRIPTION_Y, "Not yet unlocked.", 1);
    }
    gauntletUi_portraitAsset(HeroMenuInfos[hero].portrait, PORTRAIT_X, PORTRAIT_Y);
    tile = drawStats(tile, HeroMenuInfos[hero].stats);
#if HACK_skill_expansion
    if (hasKitChoice(hero))
    {
        tile = drawText(tile, vanillaKit() ? COLOR_WHITE : COLOR_GREEN, STATS_X, KIT_Y,
                        vanillaKit() ? "Vanilla" : "Expanded", 0);
    }
#endif
    {
        const char *name = ringwraith_getHeroName(hero);
        u32 length = 0;

        while (name[length] != '\0')
        {
            length++;
        }
        font_setStyle(length > NAME_LARGE_MAX ? FONT_STYLE_8PX_B : FONT_STYLE_11PX_A, COLOR_YELLOW,
                      GAUNTLET_UI_TEXT_BACKGROUND);
        tile = font_drawString(tile, SIDE_CENTER_X, NAME_Y + (length > NAME_LARGE_MAX ? 2 : 0), name, 1);
    }
    remaining = (char *)HeroMenuInfos[hero].description;
    font_setStyle(FONT_STYLE_8PX_B, COLOR_WHITE, GAUNTLET_UI_TEXT_BACKGROUND);
    font_setLineHeight(CELL);
    return font_drawLinesInBox(tile, (SIDE_X + 1) * CELL + 2, DESCRIPTION_Y, (SIDE_W - 2) * CELL - 4,
                               DESCRIPTION_LINES * CELL, &remaining, 0);
}

static void drawScreen(void)
{
    u8 heroes[HACK_HERO_COUNT];
    u32 count = groupHeroes(heroes);
    u32 first = ARG_CURSOR / CARDS_PER_PAGE * CARDS_PER_PAGE;
    u32 pages = (count + CARDS_PER_PAGE - 1) / CARDS_PER_PAGE;
    u32 hero = heroes[ARG_CURSOR];
    s32 tile = 2;
    char buffer[48];
    char *out;

    gauntletUi_beginFrame();
    font_setStyle(FONT_STYLE_11PX_A, COLOR_YELLOW, GAUNTLET_UI_TEXT_BACKGROUND);
    tile = font_drawString(tile, CENTER_X, TITLE_Y, "CHOOSE YOUR HERO", 1);
    if (HERO_GROUP_COUNT > 1)
    {
        tile = drawText(tile, COLOR_WHITE, CENTER_X - 76, GROUP_Y, "< L", 0);
        tile = drawText(tile, COLOR_WHITE, CENTER_X + 76, GROUP_Y, "R >", 2);
    }
    out = appendText(buffer, HeroGroupNames[ARG_GROUP]);
    if (pages > 1)
    {
        out = appendText(out, "  ");
        out = appendNumber(out, first / CARDS_PER_PAGE + 1);
        out = appendText(out, "/");
        appendNumber(out, pages);
    }
    tile = drawText(tile, COLOR_YELLOW, CENTER_X, GROUP_Y, buffer, 1);

    gauntletUi_panel(GRID_X, GRID_Y, GRID_W, GRID_H);
    for (u32 i = 0; i < CARDS_PER_PAGE && first + i < count; i++)
    {
        gauntletUi_card(CARD_X(i), CARD_Y(i), CARD_W, CARD_H, first + i == ARG_CURSOR);
        if (!unlocked(heroes[first + i]))
        {
            tile = drawText(tile, COLOR_WHITE, CARD_X(i) * CELL + CARD_W * CELL / 2,
                            CARD_Y(i) * CELL + CARD_H * CELL / 2 - 4, "?", 1);
        }
    }
    tile = drawDetails(tile, hero);

    out = appendText(buffer, "A Choose   B Back");
    if (HERO_GROUP_COUNT > 1)
    {
        out = appendText(out, "   L/R Group");
    }
#if HACK_skill_expansion
    if (hasKitChoice(hero))
    {
        appendText(out, "   SEL Kit");
    }
#endif
    drawText(tile, COLOR_WHITE, CENTER_X, HINT_Y, buffer, 1);
    gauntletUi_present();
    showSprites(heroes, count, first);
}

/** The card sprites' palettes darkened to level (0 untouched .. GFX_FADE_LEVELS black). */
static void dimCards(u32 level)
{
    for (u32 i = 0; i < CARDS_PER_PAGE; i++)
    {
        if (CardPalettes[i] == NULL)
        {
            continue;
        }
        for (u32 c = 0; c < 16; c++)
        {
            u16 color = CardPalettes[i][c];
            u32 r = color & 31;
            u32 g = (color >> 5) & 31;
            u32 b = (color >> 10) & 31;

            // The palette queue only drains in the main loop, so a blocking fade writes OBJ palette
            // RAM itself.
            ((volatile u16 *)OBJ_PALETTE_RAM)[i * 16 + c] =
                (u16)((r - r * level / GFX_FADE_LEVELS) | (g - g * level / GFX_FADE_LEVELS) << 5 |
                      (b - b * level / GFX_FADE_LEVELS) << 10);
        }
    }
}

/**
 * Fade the screen in or out over FADE_STEPS frames. The card sprites fade through their palettes
 * as well, since the PC renderer draws sprites without the BLDY darkening.
 */
static void fade(bool32 in)
{
    REG_BLDCNT = GFX_BLEND_ALL_TARGETS | GFX_BLEND_DARKEN;
    for (u32 step = 0; step <= FADE_STEPS; step++)
    {
        u32 level = step * GFX_FADE_LEVELS / FADE_STEPS;

        level = in ? GFX_FADE_LEVELS - level : level;
        REG_BLDY = level;
        dimCards(level);
        system_waitVBlank();
    }
}

void heroSelect_sceneEnter(void)
{
    gfx_initDisplay(0);
    sprite_resetTileAllocatorForMode(0);
    gfx_enableDisplayFlags(GFX_DISPCNT_OBJ);
    gfx_setupBg(BG_ART_BACK, MenuBackdropBgCnt);
    gfx_setBgOffset(BG_ART_BACK, 0);
    gfx_loadBgAsset(BG_ART_BACK, LanguageSelectBg3Asset, 1, 0, 0, 0);
    gfx_setupBg(BG_ART_FRONT, ART_FRONT_BGCNT);
    gfx_setBgOffset(BG_ART_FRONT, 0);
    gfx_loadBgAsset(BG_ART_FRONT, LanguageSelectBg2Asset, 0, 0, 0, 0);
    gfx_setupBg(BG_TEXT, TEXT_BGCNT);
    gfx_loadPalette(UiPalette, 0);
    gfx_setBgOffset(BG_TEXT, 0);
    font_setBgTarget(TEXT_BGCNT);
    gauntletUi_init();

    for (u32 i = 0; i < CARDS_PER_PAGE; i++)
    {
        Sprites[i] = actor_allocMain();
        sprite_setPosition(Sprites[i], (CARD_X(i) * CELL + SPRITE_X) << 16,
                           (CARD_Y(i) * CELL + SPRITE_Y) << 16);
        Sprites[i]->oam.objMode = 0;
        Sprites[i]->oam.colors256 = 0;
        Sprites[i]->oam.bgPriority = 1;
        Sprites[i]->oam.paletteNumber = i;
        Sprites[i]->flags.p = ACTOR_FLAG_ANIMATION_PLAYING;
    }
    drawScreen();
    dimCards(GFX_FADE_LEVELS);
    actor_tickAll();
    sprite_endFrame();
    fade(TRUE);
}

static void moveCursor(s32 step)
{
    u8 heroes[HACK_HERO_COUNT];
    s32 count = (s32)groupHeroes(heroes);

    ARG_CURSOR = (u32)(((s32)ARG_CURSOR + step + count * CARD_COLUMNS) % count);
    sfx_play(SFX_MENU_CURSOR_MOVE);
    drawScreen();
}

static void leave(s32 hero)
{
    pendingChoice = (u8)(hero + 1);
    game_requestSceneChangeWithArgs(SCENE_ID_SAVE_LOAD_MENU, SAVE_LOAD_MODE_BONUS_LOAD, 0);
}

void heroSelect_sceneTick(void)
{
    u16 keys = ACTIVE_MENU_KEYS_PRESSED;
    u8 heroes[HACK_HERO_COUNT];

    if (!game_isSceneChangePending())
    {
        groupHeroes(heroes);
        if (keys & A_BUTTON)
        {
            if (unlocked(heroes[ARG_CURSOR]))
            {
                sfx_play(SFX_MENU_CONFIRM);
                leave(heroes[ARG_CURSOR]);
            }
            else
            {
                sfx_play(SFX_MENU_ERROR);
            }
        }
        else if (keys & B_BUTTON)
        {
            sfx_play(SFX_MENU_BACK);
            leave(-1);
        }
#if HACK_skill_expansion
        else if ((keys & SELECT_BUTTON) && hasKitChoice(heroes[ARG_CURSOR]))
        {
            skillExpansion_players[ACTIVE_PLAYER_INDEX].newHeroVanilla = !vanillaKit();
            sfx_play(SFX_MENU_CURSOR_MOVE);
            drawScreen();
        }
#endif
        else if ((keys & (L_BUTTON | R_BUTTON)) && HERO_GROUP_COUNT > 1)
        {
            ARG_GROUP = (ARG_GROUP + (keys & R_BUTTON ? 1 : HERO_GROUP_COUNT - 1)) % HERO_GROUP_COUNT;
            ARG_CURSOR = 0;
            sfx_play(SFX_MENU_CURSOR_MOVE);
            drawScreen();
        }
        else if (keys & DPAD_LEFT)
        {
            moveCursor(-1);
        }
        else if (keys & DPAD_RIGHT)
        {
            moveCursor(1);
        }
        else if (keys & DPAD_UP)
        {
            moveCursor(-CARD_COLUMNS);
        }
        else if (keys & DPAD_DOWN)
        {
            moveCursor(CARD_COLUMNS);
        }
    }
    gauntletUi_tick();
    actor_tickAll();
    sprite_endFrame();
}

void heroSelect_sceneExit(void)
{
    fade(FALSE);
#if HACK_fast_fade
    // The next scene's fade-out would ramp from no blend and flash this black screen visible.
    fastFade_holdBlack();
#endif
    sprite_flushList(&g_ActorListMain);
    gfx_commitBgs();
    gfx_clearPaletteAnims();
    sfx_stopAllOnExit();
}

bool32 heroSelect_committing(void)
{
    return committing;
}

bool32 heroSelect_hasPendingChoice(void)
{
    return pendingChoice != 0;
}

void heroSelect_open(void)
{
    g_LastSaveSlot = (u8)SCENE_ARG_SAVE_LOAD_SELECTED_SLOT;
    game_requestSceneChangeWithArgs(MOD_SCENE_ID(HERO_SELECT), 0, 0);
}

void heroSelect_resumeSaveMenu(void)
{
    if (pendingChoice == 0)
    {
        return;
    }
    // The save menu's own hero select A: create the character and go to the bonus maps.
    SCENE_ARG_SAVE_LOAD_STATE = SAVE_LOAD_STATE_HERO_SELECT;
    SCENE_ARG_SAVE_LOAD_CURSOR = pendingChoice - 1u;
    pendingChoice = 0;
#if HACK_skill_expansion
    // The save menu's own select applies the kit on A (skillExpansion_heroSelectTick). This path
    // skips that tick, so the kit chosen on this screen is applied here.
    skillExpansion_players[ACTIVE_PLAYER_INDEX].vanillaKit = skillExpansion_heroSelectVanilla();
#endif
    committing = TRUE;
    scene_saveLoadMenu_onConfirm();
    committing = FALSE;
}

#endif
