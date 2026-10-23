#include "font.h"
#include "game.h"
#include "gba_io.h"
#include "gfx.h"
#include "input.h"
#include "memory.h"
#include "mission.h"
#include "multiplayer.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "patches/skill_expansion/skill_expansion_gimli.h"
#include "scene.h"
#include "scene/skillTree.h"
#include "sfx.h"
#include "sprite.h"
#include "text.h"

// Skill tree page 2: active rows 7..12 and passive slots 9..17. Turning the page swaps them with
// rows 0..5 and slots 0..8 of the active player, so the stock tree code draws and buys them. Row 12
// takes Herbal Healing's slot only for heroes that have one. The swap is undone when the tree
// exits.

// 0: stock skills. 1: new skills swapped into the stock rows and slots.
u8 skillExpansion_treePage;

// Tile coordinates to a sprite_setPosition coordinate.
#define TILE_TO_SPRITE_POS(tile) ((tile) << 19)

// Off-screen tile coordinates for a quick-select marker whose row is on the other page.
#define HIDDEN_MARKER_TILE_X 31
#define HIDDEN_MARKER_TILE_Y 21

#define TILE_ICON_WIDTH 4
#define TILE_ICON_HEIGHT 3
#define TILE_LEVEL_HEIGHT 1

// Passive tile of the unique hero skill on the stock page.
#define UNIQUE_PASSIVE_TILE 8

#define LEVEL_DIGIT_HERBAL_HEALING 6

#define NO_TILE 0xff

// Highest first-level requirement a passive tile can have.
#define SKILL_EXPANSION_MAX_REQUIRED_LEVEL 50

#define TITLE_WIDTH_TILES 30
#define TITLE_HEIGHT_TILES 2

// BG3 cnt the scene sets up for text.
#define SKILL_TREE_TEXT_BGCNT 0x1c08
// BG3 tiles past the font's text tiles.
#define ARROW_FIRST_TILE 0x2f0
#define ARROW_HEIGHT_TILES 2
// UiPalette yellow and black.
#define ARROW_FILL_COLOR 4u
#define ARROW_OUTLINE_COLOR 13u
// Grid rows 3..14 center on the boundary above tile row 9.
#define ARROW_TILE_Y 8
#define SCREEN_ENTRY_HFLIP 0x400

// Pressing right on these active tiles turns the page onto this passive tile, per active tile.
static const u8 ActiveRightEdgeTargets[SKILL_TREE_ACTIVE_TILE_COUNT] = {
    NO_TILE, NO_TILE, 0, NO_TILE, 3, 6,
};

// Pressing left on these passive tiles turns the page onto this active tile, per passive tile.
static const u8 PassiveLeftEdgeTargets[SKILL_TREE_PASSIVE_TILE_COUNT] = {
    2, NO_TILE, NO_TILE, 4, NO_TILE, NO_TILE, 5, NO_TILE, NO_TILE,
};

s32 skillExpansion_treeGetRequiredLevel(s32 playerIndex, s32 skill);
void skillExpansion_treeDrawDescription(void);

static void swapU8(u8 *a, u8 *b)
{
    u8 tmp = *a;

    *a = *b;
    *b = tmp;
}

/** Active slots the page turn swaps: 6 when the hero has a row 12, else 5 (Herbal Healing stays).
 */
static u32 pageActiveCount(u8 playerIndex)
{
    return skillExpansion_players[playerIndex].activeDefs[SKILL_EXPANSION_NEW_ACTIVE_COUNT - 1] !=
                   NULL
               ? SKILL_EXPANSION_NEW_ACTIVE_COUNT
               : SKILL_EXPANSION_NEW_ACTIVE_COUNT - 1;
}

/** The row an active skill row shows up as after turning the page. */
static u32 rowOnOtherPage(u8 playerIndex, u32 row)
{
    u32 count = pageActiveCount(playerIndex);

    if (row < count)
    {
        return row + SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW;
    }
    if (row >= SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW &&
        row < SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + count)
    {
        return row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW;
    }
    return row;
}

static void swapPages(u8 playerIndex)
{
    SkillExpansionPlayer *sx = &skillExpansion_players[playerIndex];
    u32 count = pageActiveCount(playerIndex);
    u32 i;

    for (i = 0; i < count; i++)
    {
        u32 row = SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + i;
        ActiveSkill tmpRow;
        const void *tmpIcon;

        memory_memcpy(&tmpRow, &sx->activeSkills[i], sizeof(ActiveSkill));
        memory_memcpy(&sx->activeSkills[i], &sx->activeSkills[row], sizeof(ActiveSkill));
        memory_memcpy(&sx->activeSkills[row], &tmpRow, sizeof(ActiveSkill));
        tmpIcon = sx->activeSkillIcons[i];
        sx->activeSkillIcons[i] = sx->activeSkillIcons[row];
        sx->activeSkillIcons[row] = tmpIcon;
        swapU8(skillExpansion_activePurchased(playerIndex, i),
               skillExpansion_activePurchased(playerIndex, row));
        swapU8(skillExpansion_activeLevel(playerIndex, i),
               skillExpansion_activeLevel(playerIndex, row));
        swapU8(skillExpansion_activeCycleEnabled(playerIndex, i),
               skillExpansion_activeCycleEnabled(playerIndex, row));
    }

    for (i = 0; i < SKILL_EXPANSION_NEW_PASSIVE_COUNT; i++)
    {
        u32 slot = HERO_PASSIVE_SKILL_COUNT + i;

        swapU8(&sx->passiveSkillIds[i], &sx->passiveSkillIds[slot]);
        swapU8((u8 *)skillExpansion_passivePurchased(playerIndex, i),
               (u8 *)skillExpansion_passivePurchased(playerIndex, slot));
        swapU8((u8 *)skillExpansion_passiveApplied(playerIndex, i),
               (u8 *)skillExpansion_passiveApplied(playerIndex, slot));
    }

    for (i = 0; i < 3; i++)
    {
        PLAYER(playerIndex).quickSelectActiveSkills[i] =
            rowOnOtherPage(playerIndex, PLAYER(playerIndex).quickSelectActiveSkills[i]);
    }
    if (!g_SaveGlobals.d.optionsFlags.d.quickSkillSelect)
    {
        PLAYER(playerIndex).selectedActiveSkill.cycleSkillIndex =
            rowOnOtherPage(playerIndex, PLAYER(playerIndex).selectedActiveSkill.cycleSkillIndex);
    }

    skillExpansion_treePage ^= 1;
}

static bool canTurnPage(u8 playerIndex)
{
    return (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE) == 0 &&
           (multiplayer_getMessageMask() & MP_MSG_FLAG_2) == 0 &&
           PLAYER(playerIndex).activeSkills == skillExpansion_players[playerIndex].activeSkills;
}

static bool activeRowIsEmpty(u8 playerIndex, u32 row)
{
    return PLAYER(playerIndex).activeSkills[row].callback == NULL;
}

/** TRUE while the tree shows a page 2 bought with the Tally instead of skill points. */
static bool tallyPage(u8 playerIndex)
{
    return skillExpansion_treePage != 0 && skillExpansion_gimliUsesTally(playerIndex);
}

/** Tally marks not yet spent on page 2 ranks. */
static u32 tallyMarks(u8 playerIndex)
{
    u32 activeOffset = skillExpansion_treePage != 0 ? 0 : SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW;
    u32 passiveOffset = skillExpansion_treePage != 0 ? 0 : HERO_PASSIVE_SKILL_COUNT;
    u32 marks = PLAYER(playerIndex).killCount / GIMLI_TALLY_KILLS_PER_MARK;
    u32 spent = 0;
    u32 i;

    for (i = 0; i < pageActiveCount(playerIndex); i++)
    {
        spent += *skillExpansion_activePurchased(playerIndex, activeOffset + i);
    }
    for (i = 0; i < SKILL_EXPANSION_NEW_PASSIVE_COUNT; i++)
    {
        s8 purchased = *skillExpansion_passivePurchased(playerIndex, passiveOffset + i);

        spent += purchased > 0 ? purchased : 0;
    }
    return marks > spent ? marks - spent : 0;
}

/** The points box: skill points, or Tally marks on a Tally page. */
static void drawPoints(u8 playerIndex)
{
    char text[16];
    u32 points = tallyPage(playerIndex) ? tallyMarks(playerIndex) : PLAYER(playerIndex).skillPoints;

    font_setStyle(FONT_STYLE_21PX, 3, -1);
    gfx_fillBgRect(3, 0xe, 0xb, 5, 4, 0);
    font_drawString(0xdd, 0x82, 0x5e, text_formatInt(points, text), 1);
}

static void placeQuickSelectMarker(u8 playerIndex, u32 slot)
{
    u32 row = PLAYER(playerIndex).quickSelectActiveSkills[slot];
    Actor *marker = g_SkillTreeQuickSelectMarkerObjs[slot];

    if (row < SKILL_TREE_ACTIVE_TILE_COUNT)
    {
        sprite_setPosition(marker, TILE_TO_SPRITE_POS(SkillTreeActiveTilePositions[row].x),
                           TILE_TO_SPRITE_POS(SkillTreeActiveTilePositions[row].y));
    }
    else
    {
        sprite_setPosition(marker, TILE_TO_SPRITE_POS(HIDDEN_MARKER_TILE_X),
                           TILE_TO_SPRITE_POS(HIDDEN_MARKER_TILE_Y));
    }
}

static void placeCursor(void)
{
    const Vector2Int *pos = SCENE_ARG_SKILL_TREE_GRID == SKILL_TREE_GRID_ACTIVE
                                ? &SkillTreeActiveTilePositions[SCENE_ARG_SKILL_TREE_CURSOR]
                                : &SkillTreePassiveTilePositions[SCENE_ARG_SKILL_TREE_CURSOR];

    sprite_setPosition(g_SkillTreeCursorObj, TILE_TO_SPRITE_POS(pos->x),
                       TILE_TO_SPRITE_POS(pos->y));
}

static void turnPage(u8 playerIndex)
{
    u32 i;

    swapPages(playerIndex);
    drawPoints(playerIndex);
    scene_skillTree_drawPassiveGrid(playerIndex);
    scene_skillTree_drawActiveGrid(playerIndex);
    if (g_SaveGlobals.d.optionsFlags.d.quickSkillSelect)
    {
        for (i = 0; i < 3; i++)
        {
            placeQuickSelectMarker(playerIndex, i);
        }
    }
}

// BG asset header flag for a 16-color palette following the 2-byte header.
#define BG_ASSET_FLAG_PALETTE 0x02
#define PALETTE_COLOR_COUNT 16

#define COLOR_RED(c) ((c) & 0x1f)
#define COLOR_BLUE(c) (((c) >> 10) & 0x1f)

// Free BG palette banks of the tree scene. Page 2 tiles of build N draw with bank
// BUILD_PALETTE_FIRST + N - 1, so each build keeps its own art ramp.
#define BUILD_PALETTE_FIRST 9
#define TREE_MAP_WIDTH 32
#define MAP_PALETTE_SHIFT 12

/**
 * Re-upload the palette that the tile draw loaded from `asset` into `paletteNumber`'s bank with
 * red and blue swapped and the icon art in `artRamp`, marking tiles on the second page.
 */
static void tintAssetPalette(const void *asset, u32 paletteNumber, const u16 *artRamp)
{
    const u8 *header = asset;
    const u16 *colors = (const u16 *)(header + 2);
    u16 tinted[PALETTE_COLOR_COUNT];
    u32 i;

    if (skillExpansion_treePage == 0 || !(header[0] & BG_ASSET_FLAG_PALETTE))
    {
        return;
    }
    tinted[0] = colors[0];
    for (i = 1; i < PALETTE_COLOR_COUNT; i++)
    {
        u16 c = colors[i];

        tinted[i] = (c & 0x03e0) | (COLOR_RED(c) << 10) | COLOR_BLUE(c);
    }
    memory_memcpy(&tinted[SKILL_EXPANSION_ICON_ART_FIRST_COLOR], artRamp,
                  SKILL_EXPANSION_ICON_ART_COLOR_COUNT * sizeof(u16));
    gfx_loadBgPaletteColors(tinted, ((header[1] >> 4) + paletteNumber) * PALETTE_COLOR_COUNT,
                            PALETTE_COLOR_COUNT);
}

/**
 * Page 2 only: give the tile at `pos` the art ramp of `build` in that build's own palette bank and
 * point the tile's icon map entries at it. Tiles without a build share the drawn bank.
 */
static void tintTileForBuild(const void *asset, u32 paletteNumber, const Vector2Int *pos, u8 build)
{
    const u8 *header = asset;
    volatile u16 *map;
    u32 bank = (header[1] >> 4) + paletteNumber;
    u32 buildBank;
    u32 x;
    u32 y;

    if (build == SKILL_EXPANSION_BUILD_NONE)
    {
        tintAssetPalette(asset, paletteNumber, skillExpansion_buildArtRamp(build, FALSE));
        return;
    }
    buildBank = BUILD_PALETTE_FIRST + build - 1;
    tintAssetPalette(asset, buildBank - (header[1] >> 4),
                     skillExpansion_buildArtRamp(build, FALSE));
    if (skillExpansion_treePage == 0)
    {
        return;
    }
    map = (volatile u16 *)BG_SCREEN_ADDR(BGCNT_GET_SCREENBASE(REG_BG2CNT));
    for (y = 0; y < TILE_ICON_HEIGHT; y++)
    {
        for (x = 0; x < TILE_ICON_WIDTH; x++)
        {
            volatile u16 *entry = &map[(pos->y + y) * TREE_MAP_WIDTH + pos->x + x];

            if ((*entry >> MAP_PALETTE_SHIFT) == bank)
            {
                *entry = (*entry & 0x0fff) | (buildBank << MAP_PALETTE_SHIFT);
            }
        }
    }
}

/** Clear a tile's icon and level and draw only its frame. */
static void drawEmptyTile(const Vector2Int *pos)
{
    gfx_fillBgRect(2, pos->x, pos->y, TILE_ICON_WIDTH, TILE_ICON_HEIGHT + TILE_LEVEL_HEIGHT, 0);
    gfx_loadBgAsset(1, SkillTreeTileFrameAsset, 0x2ef, 0, pos->x, pos->y);
}

/**
 * Move the cursor. Right past the active grid's right edge and left past the passive grid's left
 * edge turn the page.
 *
 * REPLACES (whole function): scene_skillTree_handleDpad.
 */
void skillExpansion_treeHandleDpad(void)
{
    u8 playerIndex = g_PlayerGlobals.activePlayerIndex;
    u32 oldTile = SCENE_ARG_SKILL_TREE_CURSOR & 0xff;
    u32 oldGrid = SCENE_ARG_SKILL_TREE_GRID & 0xff;
    u32 tile = oldTile;
    bool turned = FALSE;

    if (oldGrid != SKILL_TREE_GRID_ACTIVE && oldGrid != SKILL_TREE_GRID_PASSIVE)
    {
        return;
    }

    switch (MENU_KEYS_PRESSED(playerIndex) & DPAD_ANY)
    {
    case DPAD_RIGHT:
        if (oldGrid == SKILL_TREE_GRID_ACTIVE && ActiveRightEdgeTargets[oldTile] != NO_TILE &&
            canTurnPage(playerIndex))
        {
            tile = ActiveRightEdgeTargets[oldTile];
            SCENE_ARG_SKILL_TREE_GRID = SKILL_TREE_GRID_PASSIVE;
            turned = TRUE;
        }
        else
        {
            tile = scene_skillTree_moveCursorRight();
        }
        break;
    case DPAD_LEFT:
        if (oldGrid == SKILL_TREE_GRID_PASSIVE && PassiveLeftEdgeTargets[oldTile] != NO_TILE &&
            canTurnPage(playerIndex))
        {
            tile = PassiveLeftEdgeTargets[oldTile];
            SCENE_ARG_SKILL_TREE_GRID = SKILL_TREE_GRID_ACTIVE;
            turned = TRUE;
        }
        else
        {
            tile = scene_skillTree_moveCursorLeft();
        }
        break;
    case DPAD_UP:
        tile = scene_skillTree_moveCursorUp();
        break;
    case DPAD_DOWN:
        tile = scene_skillTree_moveCursorDown();
        break;
    }

    SCENE_ARG_SKILL_TREE_CURSOR = tile;
    if (turned)
    {
        turnPage(playerIndex);
    }
    if (turned || tile != oldTile || SCENE_ARG_SKILL_TREE_GRID != oldGrid)
    {
        skillExpansion_treeDrawDescription();
        placeCursor();
        sfx_play(2);
    }
}

/** Replace the scene title with the page label, or the Tally on a Tally page. */
static void drawTitle(u8 playerIndex)
{
    static const char *const PageTitles[] = {"Skills - Page 1", "Skills - Page 2"};
    static const char TallyPrefix[] = "Tally: ";
    static const char TallySuffix[] = " kills";
    char text[sizeof(TallyPrefix) + sizeof(TallySuffix) + 8];
    char digits[16];
    const char *title = PageTitles[skillExpansion_treePage];

    if (tallyPage(playerIndex))
    {
        const char *parts[] = {TallyPrefix, text_formatInt(PLAYER(playerIndex).killCount, digits),
                               TallySuffix};
        u32 length = 0;
        u32 i;

        for (i = 0; i < ARRAY_COUNT(parts); i++)
        {
            const char *c;

            for (c = parts[i]; *c != '\0' && length < sizeof(text) - 1; c++)
            {
                text[length++] = *c;
            }
        }
        text[length] = '\0';
        title = text;
    }
    gfx_fillBgRect(3, 0, 0, TITLE_WIDTH_TILES, TITLE_HEIGHT_TILES, 0);
    font_setStyle(FONT_STYLE_11PX_A, 3, 0);
    font_drawString(1, 0x78, 4, (char *)title, 1);
}

/** Pixels per row of the right-pointing arrow, from column 0. */
static const u8 ArrowRowWidths[ARROW_HEIGHT_TILES * TILE_HEIGHT] = {
    0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 0,
};

static bool arrowPixelSet(s32 x, s32 row)
{
    return row >= 0 && row < (s32)ARRAY_COUNT(ArrowRowWidths) && x >= 0 && x < ArrowRowWidths[row];
}

/** Write the arrow tiles: ARROW_FILL_COLOR inside a ARROW_OUTLINE_COLOR edge. */
static void loadArrowTiles(void)
{
    volatile u32 *dst = (volatile u32 *)(BG_CHAR_ADDR(BGCNT_GET_CHARBASE(SKILL_TREE_TEXT_BGCNT)) +
                                         ARROW_FIRST_TILE * 32);
    s32 row;

    for (row = 0; row < (s32)ARRAY_COUNT(ArrowRowWidths); row++)
    {
        u32 pixels = 0;
        s32 x;

        for (x = 0; x < TILE_WIDTH; x++)
        {
            u32 color;

            if (!arrowPixelSet(x, row))
            {
                continue;
            }
            color = arrowPixelSet(x - 1, row) && arrowPixelSet(x + 1, row) &&
                            arrowPixelSet(x, row - 1) && arrowPixelSet(x, row + 1)
                        ? ARROW_FILL_COLOR
                        : ARROW_OUTLINE_COLOR;
            pixels |= color << (x * 4);
        }
        dst[row] = pixels;
    }
}

/** Arrows at the screen edges, centered on the grids, while the page can be turned. */
static void drawPageArrows(u8 playerIndex)
{
    u32 i;

    loadArrowTiles();
    for (i = 0; i < ARROW_HEIGHT_TILES; i++)
    {
        u16 entry = canTurnPage(playerIndex) ? ARROW_FIRST_TILE + i : 0;

        gfx_fillBgRect(3, DISPLAY_TILE_WIDTH - 1, ARROW_TILE_Y + i, 1, 1, entry);
        gfx_fillBgRect(3, 0, ARROW_TILE_Y + i, 1, 1, entry ? entry | SCREEN_ENTRY_HFLIP : 0);
    }
}

void skillExpansion_treeDrawPassiveTile(u8 playerIndex, u8 passiveSkillId, u8 tile, u8 level);

/** Draw the tree's passive tiles whose first level requires `level`, from tile on. */
static u8 drawPassiveTilesAtLevel(u8 playerIndex, u8 tile, u32 level)
{
    u32 slot;

    for (slot = 0; slot < HERO_PASSIVE_SKILL_COUNT; slot++)
    {
        u8 passiveSkillId = PLAYER(playerIndex).passiveSkillIds[slot];

        if (skillExpansion_getPassiveRequiredLevel(passiveSkillId, 0) == (s32)level)
        {
            g_SkillTreePassiveTileSkillIds[tile] = passiveSkillId;
            skillExpansion_treeDrawPassiveTile(playerIndex, passiveSkillId, tile,
                                               PLAYER(playerIndex).passiveSkillLevelsApplied[slot]);
            tile++;
        }
    }
    return tile;
}

/**
 * Draw the page title, then the passive tiles in order of their first required level. Tiles past
 * the page's skills are cleared so they don't keep the other page's skills.
 *
 * REPLACES (whole function): scene_skillTree_drawPassiveGrid.
 */
void skillExpansion_treeDrawPassiveGrid(u8 playerIndex)
{
    u8 tile = 0;
    u32 level;

    drawTitle(playerIndex);
    drawPageArrows(playerIndex);
    for (level = 0; level <= SKILL_EXPANSION_MAX_REQUIRED_LEVEL; level++)
    {
        tile = drawPassiveTilesAtLevel(playerIndex, tile, level);
    }
    for (; tile < SKILL_TREE_PASSIVE_TILE_COUNT; tile++)
    {
        g_SkillTreePassiveTileSkillIds[tile] = SKILL_EXPANSION_PASSIVE_ID_NONE;
        drawEmptyTile(&SkillTreePassiveTilePositions[tile]);
    }
}

/**
 * REPLACES (whole function): scene_skillTree_drawPassiveTile.
 */
void skillExpansion_treeDrawPassiveTile(u8 playerIndex, u8 passiveSkillId, u8 tile, u8 level)
{
    const Vector2Int *pos = &SkillTreePassiveTilePositions[tile];
    u16 iconTiles = tile * 0x10 + 0x1ff;
    u16 levelTiles = tile * 0x10 + 0x20b;
    const void *icon = skillExpansion_getPassiveIcon(passiveSkillId);
    u8 build;

    if (skillExpansion_getPassiveSkill(passiveSkillId) == NULL)
    {
        drawEmptyTile(pos);
        return;
    }

    gfx_unknown08012808(2, skillExpansion_getHeroTileBgAsset(playerIndex), iconTiles, 4, pos->x,
                        pos->y);
    gfx_loadBgAsset(2, icon, iconTiles, 4, pos->x, pos->y);
    build = skillExpansion_passiveBuild(passiveSkillId);
    tintAssetPalette(icon, 4, skillExpansion_buildArtRamp(build, FALSE));
    tintTileForBuild(skillExpansion_getHeroTileBgAsset(playerIndex), 4, pos, build);
    if (tile == UNIQUE_PASSIVE_TILE && skillExpansion_treePage == 0)
    {
        if (level != 0)
        {
            gfx_unknown080127d0(2, SkillTreeUniqueLearnedAsset, 0x28b, 5, pos->x,
                                pos->y + TILE_ICON_HEIGHT);
        }
        else
        {
            gfx_unknown080127d0(2, SkillTreeUniqueUnlearnedAsset, levelTiles, 5, pos->x,
                                pos->y + TILE_ICON_HEIGHT);
        }
    }
    else
    {
        gfx_unknown080127d0(2, SkillTreeLevelDigitAssets[level], levelTiles, 5, pos->x,
                            pos->y + TILE_ICON_HEIGHT);
    }
    gfx_loadBgAsset(1, SkillTreeTileFrameAsset, 0x2ef, 0, pos->x, pos->y);
}

/**
 * REPLACES (whole function): scene_skillTree_drawActiveTile.
 */
void skillExpansion_treeDrawActiveTile(u8 playerIndex, u8 activeSkillIndex, u8 tile, u8 level)
{
    const Vector2Int *pos = &SkillTreeActiveTilePositions[tile];
    u16 iconTiles = tile * 0x10 + 0x28f;
    u16 levelTiles = tile * 0x10 + 0x29b;
    const void *icon = PLAYER(playerIndex).activeSkillIcons[activeSkillIndex];
    bool enabled = PLAYER(playerIndex).activeSkillCycleEnabled[tile] == 1 ||
                   g_SaveGlobals.d.optionsFlags.d.quickSkillSelect;

    if (activeRowIsEmpty(playerIndex, activeSkillIndex) || icon == NULL)
    {
        drawEmptyTile(pos);
        return;
    }

    if (activeSkillIndex == ACTIVE_SKILL_HERBAL_HEALING &&
        (skillExpansion_treePage == 0 || pageActiveCount(playerIndex) <= activeSkillIndex))
    {
        level = LEVEL_DIGIT_HERBAL_HEALING;
    }
    if (enabled)
    {
        gfx_unknown08012808(2, skillExpansion_getHeroTileBgAsset(playerIndex), iconTiles, 4, pos->x,
                            pos->y);
        gfx_unknown080127d0(2, icon, iconTiles, 4, pos->x, pos->y);
        tintTileForBuild(
            skillExpansion_getHeroTileBgAsset(playerIndex), 4, pos,
            skillExpansion_activeBuild(PLAYER(playerIndex).heroId,
                                       SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + activeSkillIndex));
        gfx_unknown08012808(2, SkillTreeLevelBgAsset, levelTiles, 5, pos->x,
                            pos->y + TILE_ICON_HEIGHT);
        gfx_unknown080127d0(2, SkillTreeLevelDigitAssets[level], levelTiles, 5, pos->x,
                            pos->y + TILE_ICON_HEIGHT);
    }
    else
    {
        gfx_unknown08012808(2, SkillTreeDisabledTileBgAsset, iconTiles, 7, pos->x, pos->y);
        gfx_unknown080127d0(2, icon, iconTiles, 7, pos->x, pos->y);
        tintAssetPalette(SkillTreeDisabledTileBgAsset, 7, SkillExpansionPage2DisabledArtRamp);
        gfx_unknown08012808(2, SkillTreeDisabledLevelBgAsset, levelTiles, 8, pos->x,
                            pos->y + TILE_ICON_HEIGHT);
        gfx_unknown080127d0(2, SkillTreeLevelDigitAssets[level], levelTiles, 8, pos->x,
                            pos->y + TILE_ICON_HEIGHT);
    }
    gfx_loadBgAsset(1, SkillTreeTileFrameAsset, 0x2ef, 0, pos->x, pos->y);
}

/**
 * Page 2 rows use their base row's level requirements. Empty rows and slots can't be bought.
 *
 * REPLACES (whole function): scene_skillTree_getRequiredLevel.
 */
s32 skillExpansion_treeGetRequiredLevel(s32 playerIndex, s32 skill)
{
    HeroId heroId = PLAYER(playerIndex).heroId;
    const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(heroId);

    if (SCENE_ARG_SKILL_TREE_GRID == SKILL_TREE_GRID_ACTIVE)
    {
        u32 row = skill;
        u32 ruleRow = row;

        if (row >= SKILL_TREE_ACTIVE_TILE_COUNT || heroId >= SKILL_EXPANSION_HERO_COUNT ||
            activeRowIsEmpty(playerIndex, row))
        {
            return -1;
        }
        if (skillExpansion_treePage != 0 && row < pageActiveCount(playerIndex))
        {
            const SkillExpansionActiveDef *def =
                skillExpansion_players[playerIndex].activeDefs[row];

            if (def == NULL)
            {
                return -1;
            }
            if (def->requiredLevels != NULL)
            {
                return def->requiredLevels[PLAYER(playerIndex).activeSkillLevelsPurchased[row]];
            }
            ruleRow = def->baseRow;
        }
        else if (kit != NULL)
        {
            return kit->requiredLevels[row][PLAYER(playerIndex).activeSkillLevelsPurchased[row]];
        }
        return ActiveSkillRequiredLevels[SkillExpansionHeroSkillBlocks[heroId]][ruleRow]
                                        [PLAYER(playerIndex).activeSkillLevelsPurchased[row]];
    }
    else
    {
        u8 slot = skillExpansion_findPassiveSlot(playerIndex, skill);

        if (slot == SKILL_EXPANSION_PASSIVE_SLOT_NONE)
        {
            return -1;
        }
        return skillExpansion_getPassiveRequiredLevel(
            skill, *skillExpansion_passivePurchased(playerIndex, slot));
    }
}

/**
 * Page 2 rows show their own description, or their base row's. Empty rows and slots show none.
 *
 * REPLACES (whole function): scene_skillTree_drawDescription.
 */
void skillExpansion_treeDrawDescription(void)
{
    u8 playerIndex = g_PlayerGlobals.activePlayerIndex;
    u32 tile = SCENE_ARG_SKILL_TREE_CURSOR;
    u32 heroId = PLAYER(playerIndex).heroId;
    const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(heroId);
    u32 skill;
    u32 textId;
    const char *description = NULL;
    s32 requiredLevel;

    font_setStyle(FONT_STYLE_8PX_A, 3, -1);
    gfx_fillBgRect(3, 2, 0x10, 0x1a, 4, 0);

    if (SCENE_ARG_SKILL_TREE_GRID == SKILL_TREE_GRID_ACTIVE)
    {
        u32 textRow = tile;

        if (activeRowIsEmpty(playerIndex, tile))
        {
            return;
        }
        if (skillExpansion_treePage != 0 && tile < pageActiveCount(playerIndex))
        {
            const SkillExpansionActiveDef *def =
                skillExpansion_players[playerIndex].activeDefs[tile];

            if (def == NULL)
            {
                return;
            }
            textRow = def->baseRow;
            description = def->description;
        }
        else if (kit != NULL)
        {
            description = kit->descriptions[tile];
        }
        else
        {
            description = skillExpansion_getStockActiveDescription(heroId, tile);
        }
        skill = tile;
        textId = ActiveSkillDescriptionBaseTextIds[kit != NULL ? kit->artHeroId : heroId] + textRow;
    }
    else
    {
        skill = g_SkillTreePassiveTileSkillIds[tile];
        if (skillExpansion_getPassiveSkill(skill) == NULL)
        {
            return;
        }
        textId = TEXT_ID_PASSIVE_SKILL_DESCRIPTION_BASE + skill;
        description = skillExpansion_getPassiveDescription(skill);
    }

    requiredLevel = skillExpansion_treeGetRequiredLevel(playerIndex, skill);
    if (requiredLevel == -1)
    {
        font_setTemplateArg(text_getString(TEXT_ID_SKILL_TREE_MAXED), 0);
    }
    else
    {
        font_setIntTemplateArg(
            tallyPage(playerIndex) ? requiredLevel * GIMLI_TALLY_KILLS_PER_STEP : requiredLevel, 0);
    }
    if (SCENE_ARG_SKILL_TREE_GRID == SKILL_TREE_GRID_ACTIVE)
    {
        font_setIntTemplateArg(skill_active_getSpiritCost(playerIndex, skill), 1);
    }
    if (description == NULL)
    {
        description = text_getString(textId);
    }
    font_drawLines(0x3d, 0x10, 0x83, 0xd0, (char *)description, 0);
}

/** Buy a rank of the page 2 skill under the cursor with a Tally mark. */
static void buyWithTally(u8 playerIndex)
{
    u32 tile = SCENE_ARG_SKILL_TREE_CURSOR;
    bool active = SCENE_ARG_SKILL_TREE_GRID == SKILL_TREE_GRID_ACTIVE;
    u32 skill;
    s32 required;

    if (active)
    {
        if (tile >= pageActiveCount(playerIndex) || activeRowIsEmpty(playerIndex, tile))
        {
            sfx_play(SFX_MENU_ERROR);
            return;
        }
        skill = tile;
    }
    else if (SCENE_ARG_SKILL_TREE_GRID == SKILL_TREE_GRID_PASSIVE)
    {
        skill = g_SkillTreePassiveTileSkillIds[tile];
        if (skillExpansion_findPassiveSlot(playerIndex, skill) == SKILL_EXPANSION_PASSIVE_SLOT_NONE)
        {
            sfx_play(SFX_MENU_ERROR);
            return;
        }
    }
    else
    {
        return;
    }
    required = skillExpansion_treeGetRequiredLevel(playerIndex, skill);
    if (required == -1 || tallyMarks(playerIndex) == 0 ||
        PLAYER(playerIndex).killCount < (u32)required * GIMLI_TALLY_KILLS_PER_STEP)
    {
        sfx_play(SFX_MENU_ERROR);
        return;
    }
    if (active)
    {
        skill_active_addLevels(playerIndex, skill, 1);
        skillExpansion_treeDrawActiveTile(playerIndex, skill, tile,
                                          *skillExpansion_activeLevel(playerIndex, skill));
    }
    else
    {
        u8 slot;

        skill_passive_addLevels(playerIndex, skill, 1);
        slot = skillExpansion_findPassiveSlot(playerIndex, skill);
        skillExpansion_treeDrawPassiveTile(playerIndex, skill, tile,
                                           *skillExpansion_passiveApplied(playerIndex, slot));
    }
    drawPoints(playerIndex);
    skillExpansion_treeDrawDescription();
    sfx_play(active ? SFX_SKILL_TREE_BUY_ACTIVE : SFX_SKILL_TREE_BUY_PASSIVE);
}

/**
 * Buy with a Tally mark on a Tally page, else with a skill point.
 *
 * REPLACES (whole function): scene_skillTree_handleA.
 */
void skillExpansion_treeHandleA(void)
{
    u8 playerIndex = g_PlayerGlobals.activePlayerIndex;

    if (tallyPage(playerIndex))
    {
        buyWithTally(playerIndex);
        return;
    }
    scene_skillTree_buySkill(playerIndex);
}

/**
 * Create the three quick-select markers on their rows' tiles, or off screen for rows on the
 * other page.
 *
 * REPLACES (whole function): scene_skillTree_createQuickSelectMarkers.
 */
void skillExpansion_treeCreateQuickSelectMarkers(void)
{
    u8 playerIndex = g_PlayerGlobals.activePlayerIndex;
    u32 i;

    for (i = 0; i < 3; i++)
    {
        Actor *marker = actor_allocOverlay();

        g_SkillTreeQuickSelectMarkerObjs[i] = marker;
        marker->type = 1;
        placeQuickSelectMarker(playerIndex, i);
        marker->flags.p = ACTOR_FLAG_RENDER | ACTOR_FLAG_ANIMATION_PLAYING;
        marker->oam.objMode = 0;
        marker->oam.colors256 = 0;
        marker->oam.paletteNumber = 5;
        marker->oam.bgPriority = 0;
        sprite_setAnimation(marker, &SkillTreeQuickSelectMarkerAnimations[i]);
    }
}

/**
 * Bind the row under the cursor to the next quick-select slot. A row already bound to another
 * slot trades places with that slot's row.
 *
 * REPLACES (whole function): scene_skillTree_assignQuickSelect.
 */
void skillExpansion_treeAssignQuickSelect(void)
{
    u8 playerIndex = g_PlayerGlobals.activePlayerIndex;
    u8 *slots = PLAYER(playerIndex).quickSelectActiveSkills;
    u32 row = SCENE_ARG_SKILL_TREE_CURSOR;
    u32 next = g_SkillTreeNextQuickSelectMarker;
    u32 i;

    if (activeRowIsEmpty(playerIndex, row))
    {
        sfx_play(5);
        return;
    }

    for (i = 0; i < 3; i++)
    {
        if (slots[i] == row)
        {
            slots[i] = slots[next];
            placeQuickSelectMarker(playerIndex, i);
            break;
        }
    }
    slots[next] = row;
    placeQuickSelectMarker(playerIndex, next);

    g_SkillTreeNextQuickSelectMarker = next + 1 < 3 ? next + 1 : 0;
    sfx_play(3);
}

/**
 * Turn back to the stock page, then run the stock exit.
 *
 * REPLACES (whole function): scene_skillTree_exit.
 */
void skillExpansion_treeExit(void)
{
    SceneId next;

    if (skillExpansion_treePage != 0)
    {
        swapPages(g_PlayerGlobals.activePlayerIndex);
    }

    if ((g_GameFlags.p & GAME_FLAG_COOP_ACTIVE) == 0 ||
        game_getNextSceneId() == SCENE_ID_GRIPTONITE_LOGO || (g_GameFlags.p & GAME_FLAG_31) != 0)
    {
        gfx_fadeOutToBlack(0x3f);
        if (game_getNextSceneId() == SCENE_ID_GRIPTONITE_LOGO)
        {
            sprite_flushList(&g_ActorListMain);
            mission_unknown0803915c();
            mission_unknown08039198();
        }
    }
    else
    {
        gfx_fadeOutToBlackCoop(0x3f);
    }
    sprite_flushList(&g_ActorListOverlay);
    g_GameFlags.p &= ~GAME_FLAG_18;
    next = game_getNextSceneId();
    if (next == SCENE_ID_GRIPTONITE_LOGO)
    {
        g_GameFlags.p &= ~(GAME_FLAG_OVERLAY | GAME_FLAG_WORLD_PARKED);
    }
    else
    {
        g_GameFlags.p |= GAME_FLAG_20;
    }
    gfx_commitBgs();
    gfx_clearPaletteAnims();
    sfx_stopAllOnExit();
}
