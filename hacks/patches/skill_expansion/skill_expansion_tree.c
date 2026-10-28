#if HACK_legacy_skills
#include "patches/legacy_skills/legacy_skills.h"
#endif
#include "font.h"
#include "game.h"
#include "gba_io.h"
#include "gfx.h"
#include "input.h"
#include "memory.h"
#include "mission.h"
#include "multiplayer.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "patches/skill_expansion/skill_expansion_coop.h"
#include "patches/skill_expansion/skill_expansion_gimli.h"
#include "scene.h"
#include "scene/gameplay.h"
#include "scene/skillTree.h"
#include "sfx.h"
#include "sprite.h"
#include "text.h"

// Skill tree page 2: active rows 7..12 and passive slots 9..17. While page 2 shows, the tree swaps
// them with rows 0..5 and slots 0..8 of the active player around its input handling and drawing,
// so the stock tree code draws them. Row 12 takes Herbal Healing's slot only for heroes that have
// one. Actor ticks, the HUD and co-op messages run between the swaps and see every row in its own
// place, so purchases are applied by unswapped row and both co-op consoles apply the same ones.

// 0: stock skills. 1: the new skills, swapped in while the tree handles input and draws. 2 (PC
// edition): the Two Towers skills of an imported hero, its active rows swapped in the same way.
u8 skillExpansion_treePage;

#define TREE_PAGE_STOCK 0
#define TREE_PAGE_EXPANDED 1
#define TREE_PAGE_LEGACY 2
#define TREE_PAGE_COUNT 3

#if SKILL_EXPANSION_LEGACY_ACTIVE_COUNT
#define LEGACY_PAGE_SHOWN() (skillExpansion_treePage == TREE_PAGE_LEGACY)
#else
#define LEGACY_PAGE_SHOWN() FALSE
#endif

// TRUE when an applied action changed the active player's tree, which redraws on its next tick.
static bool redrawPending;

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

static const s8 HerbalHealingRequiredLevels[] = {1, 5, 10, 15, 20, -1};

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
static u32 pageActiveCountOf(u8 playerIndex, u32 page)
{
#if SKILL_EXPANSION_LEGACY_ACTIVE_COUNT
    // The legacy page shows the first six actives in the active grid, a seventh with the passives.
    if (page == TREE_PAGE_LEGACY)
    {
        u32 count = legacySkills_activeCount();

        return count < SKILL_TREE_ACTIVE_TILE_COUNT ? count : SKILL_TREE_ACTIVE_TILE_COUNT;
    }
#endif
    (void)page;
    return skillExpansion_players[playerIndex].activeDefs[SKILL_EXPANSION_NEW_ACTIVE_COUNT - 1] !=
                   NULL
               ? SKILL_EXPANSION_NEW_ACTIVE_COUNT
               : SKILL_EXPANSION_NEW_ACTIVE_COUNT - 1;
}

static u32 pageActiveCount(u8 playerIndex)
{
    return pageActiveCountOf(playerIndex, skillExpansion_treePage);
}

static u32 pageFirstRow(u32 page)
{
    return page == TREE_PAGE_LEGACY ? SKILL_EXPANSION_FIRST_LEGACY_ACTIVE_ROW
                                    : SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW;
}

/** The row an active skill row shows up as while `page` is swapped in, or out again. */
static u32 rowOnPage(u8 playerIndex, u32 row, u32 page)
{
    u32 count = pageActiveCountOf(playerIndex, page);
    u32 first = pageFirstRow(page);

    if (row < count)
    {
        return row + first;
    }
    if (row >= first && row < first + count)
    {
        return row - first;
    }
    return row;
}

/** Where a row the tree shows sits outside the page swap. */
static u32 unswappedRow(u8 playerIndex, u32 row)
{
    return skillExpansion_treePage != TREE_PAGE_STOCK
               ? rowOnPage(playerIndex, row, skillExpansion_treePage)
               : row;
}

/**
 * Swap a page's rows (and page 2's passive slots) with the stock ones. Swapping twice restores
 * them.
 */
static void swapPage(u8 playerIndex, u32 page)
{
    SkillExpansionPlayer *sx = &skillExpansion_players[playerIndex];
    u32 count = pageActiveCountOf(playerIndex, page);
    u32 i;

    for (i = 0; i < count; i++)
    {
        u32 row = pageFirstRow(page) + i;
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

    for (i = 0; i < SKILL_EXPANSION_NEW_PASSIVE_COUNT && page == TREE_PAGE_EXPANDED; i++)
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
            rowOnPage(playerIndex, PLAYER(playerIndex).quickSelectActiveSkills[i], page);
    }
    if (!g_SaveGlobals.d.optionsFlags.d.quickSkillSelect)
    {
        PLAYER(playerIndex).selectedActiveSkill.cycleSkillIndex = rowOnPage(
            playerIndex, PLAYER(playerIndex).selectedActiveSkill.cycleSkillIndex, page);
    }
}

/** Swap the shown page in or out, around the tree's input handling and drawing. */
static void swapShownPage(u8 playerIndex)
{
    if (skillExpansion_treePage != TREE_PAGE_STOCK)
    {
        swapPage(playerIndex, skillExpansion_treePage);
    }
}

static bool canTurnPage(u8 playerIndex)
{
    // A vanilla hero (skill_expansion_modes.c) has nothing on page 2.
    return PLAYER(playerIndex).activeSkills == skillExpansion_players[playerIndex].activeSkills &&
           skillExpansion_playerExpanded(playerIndex);
}

static bool pageAvailable(u8 playerIndex, u32 page)
{
    switch (page)
    {
    case TREE_PAGE_STOCK:
        return TRUE;
    case TREE_PAGE_EXPANDED:
        return canTurnPage(playerIndex);
#if SKILL_EXPANSION_LEGACY_ACTIVE_COUNT
    case TREE_PAGE_LEGACY:
        return PLAYER(playerIndex).activeSkills ==
                   skillExpansion_players[playerIndex].activeSkills &&
               legacySkills_loaded(playerIndex);
#endif
    }
    return FALSE;
}

/** The page after (step 1) or before (step -1) the shown one, skipping pages the hero lacks. */
static u32 nextPage(u8 playerIndex, s32 step)
{
    u32 page = skillExpansion_treePage;
    u32 i;

    for (i = 0; i < TREE_PAGE_COUNT; i++)
    {
        page = (page + TREE_PAGE_COUNT + step) % TREE_PAGE_COUNT;
        if (pageAvailable(playerIndex, page))
        {
            return page;
        }
    }
    return skillExpansion_treePage;
}

static bool canTurn(u8 playerIndex)
{
    return nextPage(playerIndex, 1) != skillExpansion_treePage;
}

static bool activeRowIsEmpty(u8 playerIndex, u32 row)
{
    return PLAYER(playerIndex).activeSkills[row].callback == NULL;
}

/** TRUE while the tree shows a page 2 bought with the Tally instead of skill points. */
static bool tallyPage(u8 playerIndex)
{
    return skillExpansion_treePage == TREE_PAGE_EXPANDED &&
           skillExpansion_gimliUsesTally(playerIndex);
}

/**
 * Kills the bought page 2 ranks cost, each at the kill requirement it was listed with.
 *
 * @param firstRow the row page 2's first active skill is in, 0 while swapped in
 * @param firstSlot the slot page 2's first passive skill is in, 0 while swapped in
 */
static u32 tallySpent(u8 playerIndex, u32 firstRow, u32 firstSlot)
{
    u32 spent = 0;
    u32 i;
    u32 rank;

    for (i = 0; i < pageActiveCount(playerIndex); i++)
    {
        const SkillExpansionActiveDef *def = skillExpansion_players[playerIndex].activeDefs[i];
        u32 purchased = *skillExpansion_activePurchased(playerIndex, firstRow + i);

        if (def == NULL || def->requiredLevels == NULL)
        {
            continue;
        }
        for (rank = 0; rank < purchased && rank < PASSIVE_SKILL_MAX_LEVEL; rank++)
        {
            spent += def->requiredLevels[rank] * GIMLI_TALLY_KILLS_PER_STEP;
        }
    }
    for (i = 0; i < SKILL_EXPANSION_NEW_PASSIVE_COUNT; i++)
    {
        u32 slot = firstSlot + i;
        s8 purchased = *skillExpansion_passivePurchased(playerIndex, slot);

        for (rank = 0; (s32)rank < purchased && rank < PASSIVE_SKILL_MAX_LEVEL; rank++)
        {
            spent += skillExpansion_getPassiveRequiredLevel(
                         skillExpansion_players[playerIndex].passiveSkillIds[slot], rank) *
                     GIMLI_TALLY_KILLS_PER_STEP;
        }
    }
    return spent;
}

/** Tally kills not yet spent on page 2 ranks. The parameters are as for tallySpent. */
static u32 unspentTally(u8 playerIndex, u32 firstRow, u32 firstSlot)
{
    u32 spent = tallySpent(playerIndex, firstRow, firstSlot);
    u32 kills = PLAYER(playerIndex).killCount;

    return kills > spent ? kills - spent : 0;
}

/** Tally kills not yet spent, while page 2 is swapped in. */
static u32 tallyKills(u8 playerIndex)
{
    return unspentTally(playerIndex, 0, 0);
}

/** The points box: skill points, or Tally marks on a Tally page. */
static void drawPoints(u8 playerIndex)
{
    char text[16];
    u32 points = tallyPage(playerIndex) ? tallyKills(playerIndex) / GIMLI_TALLY_KILLS_PER_MARK
                                        : PLAYER(playerIndex).skillPoints;

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

/** Draw the points box, both grids and the quick-select markers. */
static void drawPage(u8 playerIndex)
{
    u32 i;

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

/**
 * Turn to the next page (step 1) or the one before (step -1). Runs with the shown page swapped in,
 * and leaves the new one swapped in.
 */
static void turnPage(u8 playerIndex, s32 step)
{
    u32 page = nextPage(playerIndex, step);

    swapShownPage(playerIndex);
    skillExpansion_treePage = page;
    swapShownPage(playerIndex);
    drawPage(playerIndex);
}

// BG asset header flag for a 16-color palette following the 2-byte header.
#define BG_ASSET_FLAG_PALETTE 0x02
#define PALETTE_COLOR_COUNT 16

#define COLOR_RED(c) ((c) & 0x1f)
#define COLOR_BLUE(c) (((c) >> 10) & 0x1f)

// Free BG palette banks of the tree scene. Page 2 tiles of build N draw with bank
// BUILD_PALETTE_FIRST + N - 1, so each build keeps its own art ramp.
#define BUILD_PALETTE_FIRST 9
// Page 2 tiles without a build. Builds 1..6 and icon color groups 0..5 use banks 9..14.
#define NO_BUILD_PALETTE 15
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

    if (skillExpansion_treePage != TREE_PAGE_EXPANDED || !(header[0] & BG_ASSET_FLAG_PALETTE))
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

/** Point the icon map entries of the tile at `pos` that use bank `from` at bank `to`. */
static void moveTileToBank(const Vector2Int *pos, u32 from, u32 to)
{
    volatile u16 *map = (volatile u16 *)BG_SCREEN_ADDR(BGCNT_GET_SCREENBASE(REG_BG2CNT));
    u32 x;
    u32 y;

    for (y = 0; y < TILE_ICON_HEIGHT; y++)
    {
        for (x = 0; x < TILE_ICON_WIDTH; x++)
        {
            volatile u16 *entry = &map[(pos->y + y) * TREE_MAP_WIDTH + pos->x + x];

            if ((*entry >> MAP_PALETTE_SHIFT) == from)
            {
                *entry = (*entry & 0x0fff) | (to << MAP_PALETTE_SHIFT);
            }
        }
    }
}

/**
 * Page 2 only: give the tile at `pos` the art ramp of `build` in that build's own palette bank and
 * point the tile's icon map entries at it.
 */
static void tintTileForBuild(const void *asset, u32 paletteNumber, const Vector2Int *pos, u8 build)
{
    const u8 *header = asset;
    u32 buildBank;

    // Every tile draws into the same bank, so a tint there would recolor the whole page.
    buildBank = build == SKILL_EXPANSION_BUILD_NONE ? NO_BUILD_PALETTE
                                                    : BUILD_PALETTE_FIRST + build - 1;
    tintAssetPalette(asset, buildBank - (header[1] >> 4),
                     skillExpansion_buildArtRamp(build, FALSE));
    if (skillExpansion_treePage != TREE_PAGE_EXPANDED)
    {
        return;
    }
    moveTileToBank(pos, (header[1] >> 4) + paletteNumber, buildBank);
}

/**
 * Draw the icon at `pos` in its own colors when it has them, in the bank of its color group with
 * the frame colors of `asset`. Other icons get the art ramp of `build` on page 2.
 */
static void colorTile(const void *asset, u32 paletteNumber, const void *icon,
                      const Vector2Int *pos, u8 build)
{
    const SkillExpansionIconColors *own = skillExpansion_iconColors(icon);
    const u8 *header = asset;
    u16 colors[PALETTE_COLOR_COUNT];
    u32 bank;

    if (own == NULL || !(header[0] & BG_ASSET_FLAG_PALETTE))
    {
        tintTileForBuild(asset, paletteNumber, pos, build);
        return;
    }
    bank = BUILD_PALETTE_FIRST + own->group;
    memory_memcpy(colors, header + 2, sizeof(colors));
    memory_memcpy(&colors[PALETTE_COLOR_COUNT - SKILL_EXPANSION_ICON_OWN_COLOR_COUNT], own->colors,
                  SKILL_EXPANSION_ICON_OWN_COLOR_COUNT * sizeof(u16));
    gfx_loadBgPaletteColors(colors, bank * PALETTE_COLOR_COUNT, PALETTE_COLOR_COUNT);
    moveTileToBank(pos, (header[1] >> 4) + paletteNumber, bank);
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
    s32 turned = 0; // 1: to the next page, -1: to the one before

    if (oldGrid != SKILL_TREE_GRID_ACTIVE && oldGrid != SKILL_TREE_GRID_PASSIVE)
    {
        return;
    }

    switch (MENU_KEYS_PRESSED(playerIndex) & DPAD_ANY)
    {
    case DPAD_RIGHT:
        if (oldGrid == SKILL_TREE_GRID_ACTIVE && ActiveRightEdgeTargets[oldTile] != NO_TILE &&
            canTurn(playerIndex))
        {
            tile = ActiveRightEdgeTargets[oldTile];
            SCENE_ARG_SKILL_TREE_GRID = SKILL_TREE_GRID_PASSIVE;
            turned = 1;
        }
        else
        {
            tile = scene_skillTree_moveCursorRight();
        }
        break;
    case DPAD_LEFT:
        if (oldGrid == SKILL_TREE_GRID_PASSIVE && PassiveLeftEdgeTargets[oldTile] != NO_TILE &&
            canTurn(playerIndex))
        {
            tile = PassiveLeftEdgeTargets[oldTile];
            SCENE_ARG_SKILL_TREE_GRID = SKILL_TREE_GRID_ACTIVE;
            turned = -1;
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
        turnPage(playerIndex, turned);
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
    static const char *const PageTitles[TREE_PAGE_COUNT] = {"Skills - Page 1", "Skills - Page 2",
                                                            "The Two Towers"};
    static const char TallyPrefix[] = "Tally: ";
    static const char TallySuffix[] = " kills";
    char text[sizeof(TallyPrefix) + sizeof(TallySuffix) + 8];
    char digits[16];
    const char *title = PageTitles[skillExpansion_treePage];

    if (tallyPage(playerIndex))
    {
        const char *parts[] = {TallyPrefix, text_formatInt(tallyKills(playerIndex), digits),
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
        u16 entry = canTurn(playerIndex) ? ARROW_FIRST_TILE + i : 0;

        gfx_fillBgRect(3, DISPLAY_TILE_WIDTH - 1, ARROW_TILE_Y + i, 1, 1, entry);
        gfx_fillBgRect(3, 0, ARROW_TILE_Y + i, 1, 1, entry ? entry | SCREEN_ENTRY_HFLIP : 0);
    }
}

void skillExpansion_treeDrawPassiveTile(u8 playerIndex, u8 passiveSkillId, u8 tile, u8 level);

#if SKILL_EXPANSION_LEGACY_ACTIVE_COUNT
// The legacy page's passive grid: the Two Towers passives, then a seventh active that has no
// active tile.

/** The Two Towers skill a passive tile of the legacy page shows, or -1. */
static s32 legacyPassiveTileSkill(u32 tile)
{
    u32 passives = legacySkills_passiveCount();

    if (tile < passives)
    {
        return tile;
    }
    if (tile == passives && legacySkills_activeCount() > SKILL_TREE_ACTIVE_TILE_COUNT)
    {
        return passives + SKILL_TREE_ACTIVE_TILE_COUNT;
    }
    return -1;
}

static void drawLegacyPassiveGrid(u8 playerIndex)
{
    u32 tile;

    for (tile = 0; tile < SKILL_TREE_PASSIVE_TILE_COUNT; tile++)
    {
        const Vector2Int *pos = &SkillTreePassiveTilePositions[tile];
        s32 skill = legacyPassiveTileSkill(tile);
        u16 iconTiles = tile * 0x10 + 0x1ff;
        u16 levelTiles = tile * 0x10 + 0x20b;
        const void *icon;
        u32 level;

        g_SkillTreePassiveTileSkillIds[tile] = SKILL_EXPANSION_PASSIVE_ID_NONE;
        if (skill < 0)
        {
            drawEmptyTile(pos);
            continue;
        }
        icon = legacySkills_icon(skill);
        if (icon == NULL)
        {
            icon = (u32)skill < legacySkills_passiveCount()
                       ? skillExpansion_getPassiveIcon(
                             PLAYER(playerIndex).passiveSkillIds[skill % HERO_PASSIVE_SKILL_COUNT])
                       : skillExpansion_players[playerIndex]
                             .activeSkillIcons[SKILL_EXPANSION_FIRST_LEGACY_ACTIVE_ROW +
                                               SKILL_TREE_ACTIVE_TILE_COUNT];
        }
        if (icon == NULL)
        {
            drawEmptyTile(pos);
            continue;
        }
        level = legacySkills_level(skill);
        if (level > ACTIVE_SKILL_MAX_LEVEL)
        {
            level = ACTIVE_SKILL_MAX_LEVEL;
        }
        gfx_unknown08012808(2, skillExpansion_getHeroTileBgAsset(playerIndex), iconTiles, 4,
                            pos->x, pos->y);
        gfx_loadBgAsset(2, icon, iconTiles, 4, pos->x, pos->y);
        gfx_unknown080127d0(2, SkillTreeLevelDigitAssets[level], levelTiles, 5, pos->x,
                            pos->y + TILE_ICON_HEIGHT);
        gfx_loadBgAsset(1, SkillTreeTileFrameAsset, 0x2ef, 0, pos->x, pos->y);
    }
}

/** The legacy page's description: the Two Towers text of the skill under the cursor. */
static void drawLegacyDescription(void)
{
    u32 tile = SCENE_ARG_SKILL_TREE_CURSOR;
    s32 skill = -1;
    const char *text;

    if (SCENE_ARG_SKILL_TREE_GRID != SKILL_TREE_GRID_ACTIVE)
    {
        skill = legacyPassiveTileSkill(tile);
    }
    else if (tile < pageActiveCount(g_PlayerGlobals.activePlayerIndex))
    {
        skill = legacySkills_passiveCount() + tile;
    }
    text = skill >= 0 ? legacySkills_text(skill) : NULL;
    if (text != NULL)
    {
        font_drawLines(0x3d, 0x10, 0x83, 0xd0, (char *)text, 0);
    }
}
#else
static void drawLegacyPassiveGrid(u8 playerIndex)
{
    (void)playerIndex;
}

static void drawLegacyDescription(void)
{
}
#endif

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
    if (LEGACY_PAGE_SHOWN())
    {
        drawLegacyPassiveGrid(playerIndex);
        return;
    }
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
    const PassiveSkill *skill = skillExpansion_getPassiveSkill(passiveSkillId);
    u8 build;

    if (skill == NULL)
    {
        drawEmptyTile(pos);
        return;
    }

    gfx_unknown08012808(2, skillExpansion_getHeroTileBgAsset(playerIndex), iconTiles, 4, pos->x,
                        pos->y);
    gfx_loadBgAsset(2, icon, iconTiles, 4, pos->x, pos->y);
    build = skillExpansion_passiveBuild(passiveSkillId);
    tintAssetPalette(icon, 4, skillExpansion_buildArtRamp(build, FALSE));
    colorTile(skillExpansion_getHeroTileBgAsset(playerIndex), 4, icon, pos, build);
    // A skill with one level shows the single square of the stock unique passive, not five.
    if ((tile == UNIQUE_PASSIVE_TILE && skillExpansion_treePage == 0) || skill->maxLevel == 1)
    {
        if (level != 0)
        {
            gfx_unknown080127d0(2, SkillTreeUniqueLearnedAsset, levelTiles, 5, pos->x,
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

    if (skillExpansion_getHeroKit(PLAYER(playerIndex).heroId) != NULL &&
        activeSkillIndex == ACTIVE_SKILL_HERBAL_HEALING &&
        (skillExpansion_treePage == 0 || pageActiveCount(playerIndex) <= activeSkillIndex))
    {
        level = LEVEL_DIGIT_HERBAL_HEALING;
    }
    if (enabled)
    {
        gfx_unknown08012808(2, skillExpansion_getHeroTileBgAsset(playerIndex), iconTiles, 4, pos->x,
                            pos->y);
        gfx_unknown080127d0(2, icon, iconTiles, 4, pos->x, pos->y);
        colorTile(skillExpansion_getHeroTileBgAsset(playerIndex), 4, icon, pos,
                  skillExpansion_activeBuild(PLAYER(playerIndex).heroId,
                                             SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW +
                                                 activeSkillIndex));
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
 * Level the next rank of an active row needs, or -1 when it's maxed. Page 2 rows without their own
 * requirements use their base row's.
 *
 * @param row a stock row, used when def is NULL
 * @param def the row's page 2 definition, or NULL for a stock row
 * @param purchased the row's bought ranks
 */
static s32 activeRequiredLevel(u8 playerIndex, u32 row, const SkillExpansionActiveDef *def,
                               u32 purchased)
{
    HeroId heroId = PLAYER(playerIndex).heroId;
    const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(heroId);
    u32 ruleRow = row;

    if (def != NULL)
    {
        if (def->requiredLevels != NULL)
        {
            return def->requiredLevels[purchased];
        }
        ruleRow = def->baseRow;
    }
    else if (kit != NULL)
    {
        return kit->requiredLevels[row][purchased];
    }
    else if (row == ACTIVE_SKILL_HERBAL_HEALING)
    {
        return purchased < ARRAY_COUNT(HerbalHealingRequiredLevels)
                   ? HerbalHealingRequiredLevels[purchased]
                   : -1;
    }
    return ActiveSkillRequiredLevels[SkillExpansionHeroSkillBlocks[heroId]][ruleRow][purchased];
}

/** Level the next rank of a passive skill needs, or -1 when it's maxed or not the hero's. */
static s32 passiveRequiredLevel(u8 playerIndex, u32 passiveSkillId)
{
    u8 slot = skillExpansion_findPassiveSlot(playerIndex, passiveSkillId);

    if (slot == SKILL_EXPANSION_PASSIVE_SLOT_NONE)
    {
        return -1;
    }
    return skillExpansion_getPassiveRequiredLevel(
        passiveSkillId, *skillExpansion_passivePurchased(playerIndex, slot));
}

/**
 * Empty rows and slots can't be bought.
 *
 * REPLACES (whole function): scene_skillTree_getRequiredLevel.
 */
s32 skillExpansion_treeGetRequiredLevel(s32 playerIndex, s32 skill)
{
    const SkillExpansionActiveDef *def = NULL;
    u32 row = skill;

    // Legacy levels come from The Two Towers, so the legacy page sells nothing.
    if (LEGACY_PAGE_SHOWN())
    {
        return -1;
    }
    if (SCENE_ARG_SKILL_TREE_GRID != SKILL_TREE_GRID_ACTIVE)
    {
        return passiveRequiredLevel(playerIndex, skill);
    }
    if (row >= SKILL_TREE_ACTIVE_TILE_COUNT ||
        PLAYER(playerIndex).heroId >= SKILL_EXPANSION_HERO_COUNT ||
        activeRowIsEmpty(playerIndex, row))
    {
        return -1;
    }
    if (skillExpansion_treePage != 0 && row < pageActiveCount(playerIndex))
    {
        def = skillExpansion_players[playerIndex].activeDefs[row];
        if (def == NULL)
        {
            return -1;
        }
    }
    return activeRequiredLevel(playerIndex, row, def,
                               PLAYER(playerIndex).activeSkillLevelsPurchased[row]);
}

/** As skillExpansion_treeGetRequiredLevel for an active row, with page 2 not swapped in. */
static s32 unswappedRowRequiredLevel(u8 playerIndex, u32 row)
{
    const SkillExpansionActiveDef *def = skillExpansion_activeDef(playerIndex, row);

    if (PLAYER(playerIndex).heroId >= SKILL_EXPANSION_HERO_COUNT)
    {
        return -1;
    }
    if (row >= SKILL_TREE_ACTIVE_TILE_COUNT &&
        (row < SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW ||
         row >= SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + pageActiveCount(playerIndex) ||
         def == NULL || !canTurnPage(playerIndex)))
    {
        return -1;
    }
    if (activeRowIsEmpty(playerIndex, row))
    {
        return -1;
    }
    return activeRequiredLevel(playerIndex, row, def,
                               *skillExpansion_activePurchased(playerIndex, row));
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
    if (LEGACY_PAGE_SHOWN())
    {
        drawLegacyDescription();
        return;
    }

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

/**
 * Whether a rank that needs player level `required` can be paid for.
 *
 * @param tally TRUE to pay with Tally marks, else with a skill point
 * @param unspentKills the Tally kills left when tally is TRUE
 */
static bool canPay(u8 playerIndex, s32 required, bool tally, u32 unspentKills)
{
    if (required == -1)
    {
        return FALSE;
    }
    if (tally)
    {
        return unspentKills >= (u32)required * GIMLI_TALLY_KILLS_PER_STEP;
    }
    return PLAYER(playerIndex).skillPoints != 0 && required <= PLAYER(playerIndex).level + 1;
}

/**
 * Apply a skill tree action to a player's skills, with page 2 not swapped in. Co-op consoles run
 * it for both players' actions, so it checks a purchase again: a second press can be sent before
 * the first one is applied.
 *
 * @param data the action's SKILL_EXPANSION_MP_DATA_SIZE bytes, see enum SkillExpansionMpKind
 * @param local TRUE for this console's player, whose tree shows the result
 */
void skillExpansion_treeApply(u8 playerIndex, u8 kind, const u8 *data, bool local)
{
    bool tally =
        (kind == SKILL_EXPANSION_MP_BUY_ACTIVE || kind == SKILL_EXPANSION_MP_BUY_PASSIVE) &&
        data[1] != 0;
    u32 unspentKills = 0;
    s32 sfxId = -1;
    u32 i;

    if (tally)
    {
        if (!skillExpansion_gimliUsesTally(playerIndex))
        {
            return;
        }
        unspentKills = unspentTally(playerIndex, SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW,
                                    HERO_PASSIVE_SKILL_COUNT);
    }

    switch (kind)
    {
    case SKILL_EXPANSION_MP_BUY_ACTIVE:
        if ((tally && data[0] < SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW) ||
            !canPay(playerIndex, unswappedRowRequiredLevel(playerIndex, data[0]), tally,
                    unspentKills))
        {
            sfxId = SFX_MENU_ERROR;
            break;
        }
        skill_active_addLevels(playerIndex, data[0], 1);
        if (!tally)
        {
            scene_skillTree_spendSkillPoint(playerIndex);
        }
        sfxId = SFX_SKILL_TREE_BUY_ACTIVE;
        break;

    case SKILL_EXPANSION_MP_BUY_PASSIVE:
        if ((tally &&
             skillExpansion_findPassiveSlot(playerIndex, data[0]) < HERO_PASSIVE_SKILL_COUNT) ||
            !canPay(playerIndex, passiveRequiredLevel(playerIndex, data[0]), tally, unspentKills))
        {
            sfxId = SFX_MENU_ERROR;
            break;
        }
        skill_passive_addLevels(playerIndex, data[0], 1);
        if (!tally)
        {
            scene_skillTree_spendSkillPoint(playerIndex);
        }
        sfxId = SFX_SKILL_TREE_BUY_PASSIVE;
        break;

    case SKILL_EXPANSION_MP_TOGGLE_CYCLE:
        if (data[0] < SKILL_EXPANSION_ACTIVE_ROW_COUNT)
        {
            u8 *cycleEnabled = skillExpansion_activeCycleEnabled(playerIndex, data[0]);

            *cycleEnabled = *cycleEnabled == 0;
            sfxId = SFX_MENU_CONFIRM;
        }
        break;

    case SKILL_EXPANSION_MP_QUICK_SELECT:
        for (i = 0; i < 3; i++)
        {
            if (data[i] >= SKILL_EXPANSION_ACTIVE_ROW_COUNT)
            {
                return;
            }
        }
        for (i = 0; i < 3; i++)
        {
            PLAYER(playerIndex).quickSelectActiveSkills[i] = data[i];
        }
        break;

    default:
        return;
    }

    if (local)
    {
        if (sfxId != -1)
        {
            sfx_play(sfxId);
        }
        if (sfxId != SFX_MENU_ERROR)
        {
            redrawPending = TRUE;
        }
    }
}

/** TRUE when the tree's actions go to both co-op consoles as link messages. */
static bool isLinked(void)
{
    return (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE) != 0;
}

/** Redraw the tree after an applied action changed it. Page 2 is swapped in if it shows. */
static void redrawIfPending(u8 playerIndex)
{
    if (!redrawPending)
    {
        return;
    }
    redrawPending = FALSE;
    drawPage(playerIndex);
    skillExpansion_treeDrawDescription();
}

/**
 * Apply an action of the active player now, or send it to both co-op consoles. Page 2 is swapped
 * in if it shows.
 */
static void submit(u8 playerIndex, u8 kind, const u8 *data)
{
    if (isLinked())
    {
        skillExpansion_coopSend(kind, data);
        return;
    }
    swapShownPage(playerIndex);
    skillExpansion_treeApply(playerIndex, kind, data, TRUE);
    swapShownPage(playerIndex);
    redrawIfPending(playerIndex);
}

/**
 * Buy a rank of the skill under the cursor: with a Tally mark on a Tally page, else with a skill
 * point.
 *
 * REPLACES (whole function): scene_skillTree_handleA.
 */
void skillExpansion_treeHandleA(void)
{
    u8 playerIndex = g_PlayerGlobals.activePlayerIndex;
    u32 tile = SCENE_ARG_SKILL_TREE_CURSOR;
    bool tally = tallyPage(playerIndex);
    u8 data[SKILL_EXPANSION_MP_DATA_SIZE];
    u8 kind;
    s32 required;

    memory_memset(data, 0, sizeof(data));
    if (LEGACY_PAGE_SHOWN())
    {
        sfx_play(SFX_MENU_ERROR);
        return;
    }
    if (SCENE_ARG_SKILL_TREE_GRID == SKILL_TREE_GRID_ACTIVE)
    {
        kind = SKILL_EXPANSION_MP_BUY_ACTIVE;
        data[0] = unswappedRow(playerIndex, tile);
        // Herbal Healing can stay on a Tally page, but isn't bought with the Tally.
        required = tally && tile >= pageActiveCount(playerIndex)
                       ? -1
                       : skillExpansion_treeGetRequiredLevel(playerIndex, tile);
    }
    else if (SCENE_ARG_SKILL_TREE_GRID == SKILL_TREE_GRID_PASSIVE)
    {
        kind = SKILL_EXPANSION_MP_BUY_PASSIVE;
        data[0] = g_SkillTreePassiveTileSkillIds[tile];
        required = skillExpansion_treeGetRequiredLevel(playerIndex, data[0]);
    }
    else
    {
        sfx_play(SFX_MENU_ERROR);
        return;
    }
    data[1] = tally;
    if (!canPay(playerIndex, required, tally, tally ? tallyKills(playerIndex) : 0))
    {
        sfx_play(SFX_MENU_ERROR);
        return;
    }
    submit(playerIndex, kind, data);
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
 * Select on an active tile: take the row out of the cast cycle or put it back, or bind it to the
 * next quick-select slot when quick select is on.
 *
 * REPLACES (whole function): scene_skillTree_handleSelect.
 */
void skillExpansion_treeHandleSelect(void)
{
    u8 playerIndex = g_PlayerGlobals.activePlayerIndex;
    u8 *slots = PLAYER(playerIndex).quickSelectActiveSkills;
    u8 data[SKILL_EXPANSION_MP_DATA_SIZE];
    u8 bound[3];
    bool changed = FALSE;
    u32 i;

    if (SCENE_ARG_SKILL_TREE_GRID != SKILL_TREE_GRID_ACTIVE)
    {
        return;
    }
    memory_memset(data, 0, sizeof(data));
    if (!g_SaveGlobals.d.optionsFlags.d.quickSkillSelect)
    {
        data[0] = unswappedRow(playerIndex, SCENE_ARG_SKILL_TREE_CURSOR);
        submit(playerIndex, SKILL_EXPANSION_MP_TOGGLE_CYCLE, data);
        return;
    }
    memory_memcpy(bound, slots, sizeof(bound));
    skillExpansion_treeAssignQuickSelect();
    if (!isLinked())
    {
        return;
    }
    // The markers move now, but both consoles bind the rows when the message is applied.
    for (i = 0; i < 3; i++)
    {
        changed |= slots[i] != bound[i];
        data[i] = unswappedRow(playerIndex, slots[i]);
        slots[i] = bound[i];
    }
    if (changed)
    {
        skillExpansion_coopSend(SKILL_EXPANSION_MP_QUICK_SELECT, data);
    }
}

/**
 * The stock tick, with page 2 swapped in only while the tree handles input, so the actor ticks
 * see every row in its own place.
 *
 * REPLACES (whole function): scene_skillTree_tick.
 */
void skillExpansion_treeTick(void)
{
    u32 linkMessages = multiplayer_getMessageMask();
    u8 playerIndex = g_PlayerGlobals.activePlayerIndex;
    u32 keys = MENU_KEYS_PRESSED(playerIndex);
    // Grid values 3 and 4 don't leave for the backpack or the stats page with L and R.
    bool canLeave = (u32)(SCENE_ARG_SKILL_TREE_GRID - 3) >= 2;
    u32 i;

    if (linkMessages & MP_MSG_FLAG_2)
    {
        for (i = 0; i < g_PlayerGlobals.activePlayerCount; i++)
        {
            mission_tickCameraPan(i);
            gfx_updatePlayerCameraScroll(i);
        }
    }

    swapShownPage(playerIndex);
    redrawIfPending(playerIndex);
    if (keys & A_BUTTON)
    {
        skillExpansion_treeHandleA();
    }
    else if (keys & DPAD_ANY)
    {
        skillExpansion_treeHandleDpad();
    }
    else if ((keys & (L_BUTTON | R_BUTTON)) && canLeave)
    {
        g_GameFlags.p |= GAME_FLAG_REQUEST_OVERLAY;
        game_requestSceneChange((keys & L_BUTTON) ? SCENE_ID_BACKPACK : SCENE_ID_CHARACTER_STATS);
        sfx_play(SFX_MENU_CURSOR_MOVE);
    }
    else if (keys & (B_BUTTON | START_BUTTON))
    {
        game_requestSceneChangeWithArgs(SCENE_ID_GAMEPLAY, GAMEPLAY_MODE_RESUME,
                                        g_CurrentMissionId);
        sfx_play(SFX_MENU_BACK);
    }
    else if (keys & SELECT_BUTTON)
    {
        skillExpansion_treeHandleSelect();
    }
    swapShownPage(playerIndex);

    actor_tickAll();
    if (linkMessages & MP_MSG_FLAG_2)
    {
        game_tickFxAndHuds();
    }
    sprite_endFrame();
}

/**
 * Show the stock page again on the next visit, then run the stock exit.
 *
 * REPLACES (whole function): scene_skillTree_exit.
 */
void skillExpansion_treeExit(void)
{
    SceneId next;

    skillExpansion_treePage = 0;

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
