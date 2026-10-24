#include "patches/gauntlet/gauntlet_scene.h"

#include "font.h"
#include "game.h"
#include "gba_io.h"
#include "gfx.h"
#include "input.h"
#include "patches/gauntlet/gauntlet.h"
#include "patches/gauntlet/gauntlet_boons.h"
#include "patches/gauntlet/gauntlet_level.h"
#include "patches/gauntlet/gauntlet_meta.h"
#include "patches/gauntlet/gauntlet_test.h"
#include "patches/gauntlet/gauntlet_ui.h"
#include "player.h"
#include "scene.h"
#include "scene/gameplay.h"
#include "scene/languageSelect.h"
#include "sfx.h"
#include "sprite.h"
#include "variables.h"

#define BG_ART_BACK 0u
#define BG_TEXT 2u
#define BG_ART_FRONT 3u

// The save migration scene's layer setup: see save_migrate_scene.c for the charbase choices.
#define ART_FRONT_BGCNT (BGCNT_PRIORITY(2) | BGCNT_CHARBASE(1) | BGCNT_SCREENBASE(0x1eu))
#define TEXT_BGCNT GAUNTLET_UI_TEXT_BGCNT

// UiPalette bank 0 text colors (font_setStyle color index).
#define COLOR_WHITE 0
#define COLOR_YELLOW 3
#define COLOR_RED 6
#define COLOR_GREEN 9
#define COLOR_NORMAL COLOR_YELLOW
#define COLOR_SELECTED COLOR_YELLOW
#define COLOR_UNSELECTED COLOR_WHITE

static const u8 RarityColors[] = {
    [RARITY_COMMON] = COLOR_WHITE,
    [RARITY_RARE] = COLOR_GREEN,
    [RARITY_EPIC] = COLOR_RED,
    [RARITY_LEGENDARY] = COLOR_YELLOW,
};

static const char *const HeroNames[GAUNTLET_META_HERO_COUNT] = {
    "Frodo", "Legolas", "Aragorn", "Gandalf",    "Eowyn",
    "Gimli", "Sam",     "Smeagol", "Ringwraith", "Saruman",
};

#define CENTER_X 0x78
#define TITLE_Y 8
#define PROMPT_Y 24
#define RIGHT_X 222
#define LINE_PITCH 16

// Panel layout in cells. Rows 3 to 17 lie between the backdrop's red bands.
#define CELL 8
#define FULL_X 1
#define FULL_W 28
// pick pages: the offers, then the selected offer's description
#define OFFERS_Y 4
#define OFFERS_H 7
#define DETAIL_Y 11
#define DETAIL_H 7
// hub and results: portrait on the left, details on the right
#define PORTRAIT_PANEL_X 1
#define PORTRAIT_PANEL_Y 4
#define PORTRAIT_PANEL_W (GAUNTLET_UI_PORTRAIT_W + 2)
#define PORTRAIT_PANEL_H (GAUNTLET_UI_PORTRAIT_H + 2)
#define SIDE_X 10
#define SIDE_W 19
#define SIDE_Y 4
#define SIDE_H 14
#define SIDE_TEXT_X ((SIDE_X + 1) * CELL + 4)
#define SIDE_CENTER_X ((SIDE_X * 2 + SIDE_W) * CELL / 2)
#define PORTRAIT_CENTER_X ((PORTRAIT_PANEL_X * 2 + PORTRAIT_PANEL_W) * CELL / 2)
// unlocks page: the list, then one description line
#define UNLOCKS_Y 4
#define UNLOCKS_H 11
#define UNLOCK_DETAIL_Y 15
#define UNLOCK_DETAIL_H 3
#define UNLOCK_RANK_X 168

enum HubEntry
{
    HUB_BEGIN,
    HUB_UNLOCKS,
    HUB_BACK,
    HUB_ENTRY_COUNT,
};

static const char *const HubEntryNames[HUB_ENTRY_COUNT] = {"Begin the run", "Unlocks",
                                                           "Back to title"};

// Scene state in g_SceneCurrent.args, zeroed on entry except arg 0.
#define ARG_PAGE g_SceneCurrent.args[0]
#define ARG_CURSOR g_SceneCurrent.args[1]
// Pick pages: the three offers, one byte each. Results: Embers earned, bit 16 for a new best.
#define ARG_OFFERS g_SceneCurrent.args[2]
#define ARG_RESULT g_SceneCurrent.args[2]
// Hub pages: the HeroId on show.
#define ARG_HERO g_SceneCurrent.args[3]

#define RESULT_NEW_BEST (1u << 16)
#define OFFER_NONE 0xff

static const char *DetailStart;
static const char *DetailNext;

static u8 offer(u32 slot)
{
    return ARG_OFFERS >> (slot * 8);
}

static void setOffers(const u8 offers[GAUNTLET_OFFER_COUNT])
{
    ARG_OFFERS = offers[0] | offers[1] << 8 | offers[2] << 16;
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
    char digits[10];
    u32 count = 0;

    do
    {
        digits[count++] = '0' + value % 10;
        value /= 10;
    } while (value != 0);
    while (count != 0)
    {
        *out++ = digits[--count];
    }
    *out = '\0';
    return out;
}

static s32 drawTitle(s32 tile, char *title)
{
    font_setStyle(FONT_STYLE_11PX_A, COLOR_NORMAL, 0);
    return font_drawString(tile, CENTER_X, TITLE_Y, title, 1);
}

static s32 drawText(s32 tile, s32 color, s32 x, s32 y, const char *text, s32 alignment)
{
    font_setStyle(FONT_STYLE_8PX_B, color, 0);
    return font_drawString(tile, x, y, (char *)text, alignment);
}

/** A menu row in a panel: the highlight bar and cursor when selected, then the label. */
static s32 drawRow(s32 tile, s32 panelX, s32 panelW, s32 y, bool32 selected, const char *label)
{
    if (selected)
    {
        gauntletUi_highlight(panelX + 1, y / CELL, panelW - 2);
        tile = drawText(tile, COLOR_SELECTED, (panelX + 1) * CELL + 2, y, ">", 0);
    }
    return drawText(tile, selected ? COLOR_SELECTED : COLOR_UNSELECTED, (panelX + 2) * CELL + 2, y,
                    label, 0);
}

static s32 drawParagraph(s32 tile, s32 panelX, s32 panelW, s32 y, const char *text)
{
    font_setStyle(FONT_STYLE_8PX_B, COLOR_NORMAL, 0);
    font_setLineHeight(LINE_PITCH);
    return font_drawLines(tile, (panelX + 1) * CELL + 4, y, (panelW - 2) * CELL - 8, (char *)text,
                          0);
}

/** The hero's portrait in its frame, with a caption centered under it. */
static s32 drawPortrait(s32 tile, u32 heroId, const char *caption)
{
    gauntletUi_panel(PORTRAIT_PANEL_X, PORTRAIT_PANEL_Y, PORTRAIT_PANEL_W, PORTRAIT_PANEL_H);
    gauntletUi_portrait(heroId, PORTRAIT_PANEL_X + 1, PORTRAIT_PANEL_Y + 1);
    return drawText(tile, COLOR_WHITE, PORTRAIT_CENTER_X,
                    (PORTRAIT_PANEL_Y + PORTRAIT_PANEL_H + 1) * CELL, caption, 1);
}

static const char *offerName(u32 slot, char *buffer)
{
    if (ARG_PAGE == GAUNTLET_PAGE_LEVEL)
    {
        gauntletLevel_label(0, offer(slot), buffer);
        return buffer;
    }
    return gauntletBoons_name(offer(slot));
}

static const char *offerDescription(u32 slot)
{
    if (ARG_PAGE == GAUNTLET_PAGE_LEVEL)
    {
        return gauntletLevel_description(0, offer(slot));
    }
    return gauntletBoons_description(offer(slot));
}

static void drawPick(s32 tile)
{
    char buffer[48];
    u32 slot;
    char *out;
    char *description;

    gauntletUi_panel(FULL_X, OFFERS_Y, FULL_W, OFFERS_H);
    gauntletUi_panel(FULL_X, DETAIL_Y, FULL_W, DETAIL_H);
    if (ARG_PAGE == GAUNTLET_PAGE_LEVEL)
    {
        out = appendText(buffer, "LEVEL ");
        appendNumber(out, PLAYER(0).level);
        tile = drawTitle(tile, buffer);
        tile = drawText(tile, COLOR_NORMAL, CENTER_X, PROMPT_Y, "Choose a reward", 1);
    }
    else
    {
        out = appendText(buffer, "ROOM ");
        out = appendNumber(out, gauntlet_roomsCleared());
        appendText(out, " CLEARED");
        tile = drawTitle(tile, buffer);
        out = appendText(buffer, "Choose a boon");
        if (gauntlet_rerollsLeft() != 0)
        {
            out = appendText(out, "    R: reroll (");
            out = appendNumber(out, gauntlet_rerollsLeft());
            appendText(out, ")");
        }
        tile = drawText(tile, COLOR_NORMAL, CENTER_X, PROMPT_Y, buffer, 1);
    }
    for (slot = 0; slot < GAUNTLET_OFFER_COUNT; slot++)
    {
        s32 y = (OFFERS_Y + 1 + slot * 2) * CELL;

        if (offer(slot) == OFFER_NONE)
        {
            continue;
        }
        tile = drawRow(tile, FULL_X, FULL_W, y, slot == ARG_CURSOR, offerName(slot, buffer));
        if (ARG_PAGE == GAUNTLET_PAGE_BOON)
        {
            tile = drawText(tile, RarityColors[gauntletBoons_rarity(offer(slot))], RIGHT_X, y,
                            gauntletBoons_rarityName(offer(slot)), 2);
        }
    }
    font_setStyle(FONT_STYLE_8PX_B, COLOR_NORMAL, 0);
    font_setLineHeight(12);
    description = (char *)offerDescription(ARG_CURSOR);
    if (DetailStart != NULL)
    {
        description = (char *)DetailStart;
    }
    tile = font_drawLinesInBox(tile, (FULL_X + 1) * CELL + 4, (DETAIL_Y + 1) * CELL,
                               (FULL_W - 2) * CELL - 8, 36, &description, 0);
    DetailNext = *description != '\0' ? description : NULL;
    if (ARG_PAGE == GAUNTLET_PAGE_LEVEL)
    {
        gauntletLevel_progress(0, offer(ARG_CURSOR), buffer);
    }
    else
    {
        out = appendText(buffer, "Owned: ");
        out = appendNumber(out, gauntletBoons_stacks(offer(ARG_CURSOR)));
        appendText(out, "/1 -> 1/1 (unique)");
    }
    tile = drawText(tile, COLOR_WHITE, (FULL_X + 1) * CELL + 4, 132, buffer, 0);
    if (DetailNext != NULL || DetailStart != NULL)
    {
        drawText(tile, COLOR_WHITE, CENTER_X, 148, "Left/right: more details", 1);
    }
}

/** A label on the left of the side panel and its value on the right. */
static s32 drawStat(s32 tile, s32 y, const char *label, u32 value)
{
    char buffer[12];

    tile = drawText(tile, COLOR_WHITE, SIDE_TEXT_X, y, label, 0);
    appendNumber(buffer, value);
    return drawText(tile, COLOR_YELLOW, (SIDE_X + SIDE_W - 1) * CELL - 4, y, buffer, 2);
}

static void drawResults(s32 tile)
{
    char buffer[24];
    char *out;

    tile = drawTitle(tile, "THE GAUNTLET ENDS");
    tile = drawPortrait(tile, gauntlet_hero(), HeroNames[gauntlet_hero()]);
    gauntletUi_panel(SIDE_X, SIDE_Y, SIDE_W, SIDE_H);
    tile = drawStat(tile, 40, "Rooms cleared", gauntlet_roomsCleared());
    tile = drawStat(tile, 56, "Foes slain", gauntlet_kills());
    tile = drawStat(tile, 72, "Level reached", PLAYER(0).level);
    tile = drawStat(tile, 88, "Boons", gauntletBoons_ownedCount());
    out = appendText(buffer, "+");
    out = appendNumber(out, ARG_RESULT & 0xffff);
    appendText(out, " Embers");
    tile = drawText(tile, COLOR_GREEN, SIDE_CENTER_X, 104, buffer, 1);
    if (ARG_RESULT & RESULT_NEW_BEST)
    {
        tile = drawText(tile, COLOR_YELLOW, PORTRAIT_CENTER_X, 136, "New best!", 1);
    }
    gauntletUi_highlight(SIDE_X + 1, 16, SIDE_W - 2);
    drawText(tile, COLOR_WHITE, SIDE_CENTER_X, 128, "Press A", 1);
}

static void drawHub(s32 tile)
{
    GauntletMeta meta;
    char buffer[40];
    char *out;
    u32 entry;

    gauntletMeta_load(&meta);
    tile = drawTitle(tile, "THE GAUNTLET");
    tile = drawPortrait(tile, ARG_HERO, "<  L / R  >");
    gauntletUi_panel(SIDE_X, SIDE_Y, SIDE_W, SIDE_H);
    font_setStyle(FONT_STYLE_11PX_A, COLOR_YELLOW, 0);
    tile = font_drawString(tile, SIDE_CENTER_X, 40, (char *)HeroNames[ARG_HERO], 1);
    out = appendText(buffer, "Best: room ");
    appendNumber(out, meta.best[ARG_HERO]);
    tile = drawText(tile, COLOR_WHITE, SIDE_CENTER_X, 64, buffer, 1);
    out = appendText(buffer, "Embers: ");
    appendNumber(out, gauntletMeta_embers(&meta));
    tile = drawText(tile, COLOR_GREEN, SIDE_CENTER_X, 80, buffer, 1);
    for (entry = 0; entry < HUB_ENTRY_COUNT; entry++)
    {
        tile = drawRow(tile, SIDE_X, SIDE_W, 96 + entry * 16, entry == ARG_CURSOR,
                       HubEntryNames[entry]);
    }
}

static void drawUnlocks(s32 tile)
{
    GauntletMeta meta;
    char buffer[24];
    char *out;
    u32 unlock;

    gauntletMeta_load(&meta);
    tile = drawTitle(tile, "UNLOCKS");
    out = appendText(buffer, "Embers: ");
    appendNumber(out, gauntletMeta_embers(&meta));
    tile = drawText(tile, COLOR_GREEN, CENTER_X, PROMPT_Y, buffer, 1);
    gauntletUi_panel(FULL_X, UNLOCKS_Y, FULL_W, UNLOCKS_H);
    gauntletUi_panel(FULL_X, UNLOCK_DETAIL_Y, FULL_W, UNLOCK_DETAIL_H);
    for (unlock = 0; unlock < GAUNTLET_UNLOCK_COUNT; unlock++)
    {
        s32 y = (UNLOCKS_Y + 1 + unlock * 2) * CELL;
        u32 cost = gauntletMeta_unlockCost(&meta, unlock);

        tile =
            drawRow(tile, FULL_X, FULL_W, y, unlock == ARG_CURSOR, gauntletMeta_unlockName(unlock));
        out = appendNumber(buffer, meta.unlocks[unlock]);
        out = appendText(out, "/");
        appendNumber(out, gauntletMeta_unlockMaxRank(unlock));
        tile = drawText(tile, COLOR_WHITE, UNLOCK_RANK_X, y, buffer, 2);
        if (cost == 0)
        {
            tile = drawText(tile, COLOR_YELLOW, RIGHT_X, y, "MAX", 2);
            continue;
        }
        appendNumber(buffer, cost);
        tile = drawText(tile, cost <= gauntletMeta_embers(&meta) ? COLOR_GREEN : COLOR_RED, RIGHT_X,
                        y, buffer, 2);
    }
    drawText(tile, COLOR_NORMAL, (FULL_X + 1) * CELL + 4, (UNLOCK_DETAIL_Y + 1) * CELL,
             gauntletMeta_unlockDescription(ARG_CURSOR), 0);
}

static void drawBriefing(s32 tile)
{
    char buffer[48];
    char *out = appendText(buffer, "ROOM ");
    appendNumber(out, gauntlet_roomsCleared() + 1);
    tile = drawTitle(tile, buffer);
    gauntletUi_panel(FULL_X, OFFERS_Y, FULL_W, 14);
    tile = drawText(tile, COLOR_YELLOW, CENTER_X, 40, gauntlet_roomName(), 1);
    tile = drawText(tile, COLOR_WHITE, CENTER_X, 64, gauntlet_encounterName(), 1);
    tile = drawParagraph(tile, FULL_X, FULL_W, 88,
                         "Explore the map. Find and defeat three groups of defenders.");
    drawText(tile, COLOR_WHITE, CENTER_X, 128, "A: enter the room", 1);
}

static void drawScreen(void)
{
    gauntletUi_beginFrame();
    switch (ARG_PAGE)
    {
    case GAUNTLET_PAGE_BRIEFING:
        drawBriefing(2);
        break;
    case GAUNTLET_PAGE_RESULTS:
        drawResults(2);
        break;
    case GAUNTLET_PAGE_HUB:
        drawHub(2);
        break;
    case GAUNTLET_PAGE_UNLOCKS:
        drawUnlocks(2);
        break;
    default:
        drawPick(2);
        break;
    }
    gauntletUi_present();
}

static void showPage(u32 page)
{
    ARG_PAGE = page;
    ARG_CURSOR = 0;
    DetailStart = DetailNext = NULL;
    drawScreen();
}

/** Show the next page between rooms: queued level-up rewards, then the next room. */
static void nextPage(void)
{
    u8 offers[GAUNTLET_OFFER_COUNT];

    while (gauntlet_takeLevelPick())
    {
        gauntletLevel_roll(0, offers);
        if (offers[0] != LEVEL_OFFER_NONE)
        {
            setOffers(offers);
            showPage(GAUNTLET_PAGE_LEVEL);
            return;
        }
    }
    gauntlet_prepareNextRoom();
    showPage(GAUNTLET_PAGE_BRIEFING);
}

/** Roll the boon page's offers. FALSE when no boon is left to offer. */
static bool32 rollBoons(void)
{
    u8 offers[GAUNTLET_OFFER_COUNT];

    gauntletBoons_roll(offers);
    setOffers(offers);
    return offers[0] != GAUNTLET_NO_BOON;
}

static void openFirstPage(void)
{
    GauntletMeta meta;

    switch (ARG_PAGE)
    {
    case GAUNTLET_PAGE_BOON:
        if (!rollBoons())
        {
            nextPage();
            return;
        }
        break;
    case GAUNTLET_PAGE_RESULTS:
        ARG_RESULT = gauntletMeta_runReward(gauntlet_roomsCleared(), gauntlet_kills());
        if (gauntletMeta_recordRun(gauntlet_hero(), gauntlet_roomsCleared(), gauntlet_kills()))
        {
            ARG_RESULT |= RESULT_NEW_BEST;
        }
        break;
    case GAUNTLET_PAGE_HUB:
        gauntletMeta_load(&meta);
        ARG_HERO = meta.lastHero;
        break;
    }
    drawScreen();
}

/** ModSceneHandlers[MOD_SCENE_ROW_GAUNTLET].enter (mod_scenes). */
void gauntlet_sceneEnter(void)
{
    DetailStart = DetailNext = NULL;
    // Gameplay parked its world on the way out. Each room loads a fresh map instead.
    scene_gameplay_discardParkedWorld();

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

    openFirstPage();
    actor_tickAll();
    sprite_endFrame();
    gfx_fadeInFromBlack(0x3f);
}

static void moveCursor(s32 step, u32 count, bool32 skipEmptyOffers)
{
    u32 tries;

    for (tries = 0; tries < count; tries++)
    {
        ARG_CURSOR = (ARG_CURSOR + count + step) % count;
        if (!skipEmptyOffers || offer(ARG_CURSOR) != OFFER_NONE)
        {
            break;
        }
    }
    DetailStart = DetailNext = NULL;
    sfx_play(SFX_MENU_CURSOR_MOVE);
    drawScreen();
}

static void tickPick(u16 keys)
{
    if (keys & A_BUTTON)
    {
        sfx_play(SFX_MENU_CONFIRM);
        if (ARG_PAGE == GAUNTLET_PAGE_LEVEL)
        {
            gauntletLevel_apply(0, offer(ARG_CURSOR));
        }
        else
        {
            gauntletBoons_grant(offer(ARG_CURSOR));
        }
        nextPage();
    }
    else if ((keys & R_BUTTON) && ARG_PAGE == GAUNTLET_PAGE_BOON)
    {
        if (!gauntlet_useReroll())
        {
            sfx_play(SFX_MENU_ERROR);
            return;
        }
        sfx_play(SFX_MENU_CONFIRM);
        rollBoons();
        ARG_CURSOR = 0;
        DetailStart = DetailNext = NULL;
        drawScreen();
    }
    else if (keys & (DPAD_UP | DPAD_DOWN))
    {
        moveCursor(keys & DPAD_UP ? -1 : 1, GAUNTLET_OFFER_COUNT, TRUE);
    }
    else if (keys & (DPAD_LEFT | DPAD_RIGHT))
    {
        if ((keys & DPAD_LEFT) || DetailNext != NULL)
        {
            DetailStart = keys & DPAD_LEFT ? NULL : DetailNext;
            sfx_play(SFX_MENU_CURSOR_MOVE);
            drawScreen();
        }
    }
}

static void saveLastHero(void)
{
    GauntletMeta meta;

    gauntletMeta_load(&meta);
    if (meta.lastHero != ARG_HERO)
    {
        meta.lastHero = ARG_HERO;
        gauntletMeta_save(&meta);
    }
}

static void tickHub(u16 keys)
{
    if (keys & (DPAD_LEFT | DPAD_RIGHT))
    {
        ARG_HERO = (ARG_HERO + GAUNTLET_META_HERO_COUNT + (keys & DPAD_LEFT ? -1 : 1)) %
                   GAUNTLET_META_HERO_COUNT;
        sfx_play(SFX_MENU_CURSOR_MOVE);
        drawScreen();
    }
    else if (keys & (DPAD_UP | DPAD_DOWN))
    {
        moveCursor(keys & DPAD_UP ? -1 : 1, HUB_ENTRY_COUNT, FALSE);
    }
    else if (keys & B_BUTTON)
    {
        sfx_play(SFX_MENU_BACK);
        game_requestSceneChange(SCENE_ID_MAIN_MENU);
    }
    else if (keys & A_BUTTON)
    {
        switch (ARG_CURSOR)
        {
        case HUB_BEGIN:
            sfx_play(SFX_MENU_CONFIRM);
            saveLastHero();
            gauntlet_startRun(ARG_HERO);
            showPage(GAUNTLET_PAGE_BRIEFING);
            break;
        case HUB_UNLOCKS:
            sfx_play(SFX_MENU_CONFIRM);
            showPage(GAUNTLET_PAGE_UNLOCKS);
            break;
        default:
            sfx_play(SFX_MENU_BACK);
            game_requestSceneChange(SCENE_ID_MAIN_MENU);
            break;
        }
    }
}

static void tickUnlocks(u16 keys)
{
    GauntletMeta meta;

    if (keys & (DPAD_UP | DPAD_DOWN))
    {
        moveCursor(keys & DPAD_UP ? -1 : 1, GAUNTLET_UNLOCK_COUNT, FALSE);
    }
    else if (keys & B_BUTTON)
    {
        sfx_play(SFX_MENU_BACK);
        ARG_PAGE = GAUNTLET_PAGE_HUB;
        ARG_CURSOR = HUB_UNLOCKS;
        drawScreen();
    }
    else if (keys & A_BUTTON)
    {
        gauntletMeta_load(&meta);
        if (!gauntletMeta_buy(&meta, ARG_CURSOR))
        {
            sfx_play(SFX_MENU_ERROR);
            return;
        }
        sfx_play(SFX_MENU_CONFIRM);
        drawScreen();
    }
}

/** ModSceneHandlers[MOD_SCENE_ROW_GAUNTLET].tick (mod_scenes). */
void gauntlet_sceneTick(void)
{
    u16 keys = ACTIVE_MENU_KEYS_PRESSED | gauntletTest_scenePress(ARG_PAGE, ARG_CURSOR);

    if (!game_isSceneChangePending())
    {
        switch (ARG_PAGE)
        {
        case GAUNTLET_PAGE_BRIEFING:
            if (keys & A_BUTTON)
            {
                sfx_play(SFX_MENU_CONFIRM);
                gauntlet_enterNextRoom();
            }
            break;
        case GAUNTLET_PAGE_RESULTS:
            if (keys & A_BUTTON)
            {
                sfx_play(SFX_MENU_CONFIRM);
                gauntlet_finishRun();
                ARG_HERO = gauntlet_hero();
                showPage(GAUNTLET_PAGE_HUB);
            }
            break;
        case GAUNTLET_PAGE_HUB:
            tickHub(keys);
            break;
        case GAUNTLET_PAGE_UNLOCKS:
            tickUnlocks(keys);
            break;
        default:
            tickPick(keys);
            break;
        }
    }
    gauntletUi_tick();
    actor_tickAll();
    sprite_endFrame();
}

/** ModSceneHandlers[MOD_SCENE_ROW_GAUNTLET].exit (mod_scenes). */
void gauntlet_sceneExit(void)
{
    gfx_fadeOutToBlack(0x3f);
    sprite_flushList(&g_ActorListMain);
    gfx_commitBgs();
    gfx_clearPaletteAnims();
    sfx_stopAllOnExit();
}
