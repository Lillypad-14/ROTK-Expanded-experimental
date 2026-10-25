#include "patches/gauntlet/gauntlet_scene.h"

#include "font.h"
#include "game.h"
#include "gba_io.h"
#include "gfx.h"
#include "input.h"
#include "menu.h"
#include "memory.h"
#include "item.h"
#include "patches/gauntlet/gauntlet.h"
#include "patches/gauntlet/gauntlet_boons.h"
#include "patches/gauntlet/gauntlet_level.h"
#include "patches/gauntlet/gauntlet_meta.h"
#include "patches/gauntlet/gauntlet_shop.h"
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
// Door page: the doors packed 16 bits each, two then one.
#define ARG_DOORS_LOW g_SceneCurrent.args[2]
#define ARG_DOORS_HIGH g_SceneCurrent.args[3]

#define REWARD_FALLBACK (1u << 24)

#define RESULT_NEW_BEST (1u << 16)
#define OFFER_NONE 0xff

static union
{
    struct
    {
        const char *start;
        const char *next;
    } detail;
    struct
    {
        u8 offsets[2];
        u8 next[2];
        bool more[2];
        u8 doorCursor;
        u8 message;
    } shop;
} PageState;

#define DetailStart PageState.detail.start
#define DetailNext PageState.detail.next
#define ShopDoorCursor PageState.shop.doorCursor
#define ShopStatOffsets PageState.shop.offsets
#define ShopStatNext PageState.shop.next
#define ShopMore PageState.shop.more
#define ShopMessage ShopMessages[PageState.shop.message]

static const char *const ShopMessages[] = {NULL, "Bought. Stored in your backpack.",
                                           "Already sold.", "Not enough gems.",
                                           "Your backpack is full.", "Sold for gems.",
                                           "Item changed or cannot be sold.", "Gem pouch is full."};

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
    font_setStyle(FONT_STYLE_11PX_A, COLOR_NORMAL, GAUNTLET_UI_TEXT_BACKGROUND);
    return font_drawString(tile, CENTER_X, TITLE_Y, title, 1);
}

static s32 drawText(s32 tile, s32 color, s32 x, s32 y, const char *text, s32 alignment)
{
    font_setStyle(FONT_STYLE_8PX_B, color, GAUNTLET_UI_TEXT_BACKGROUND);
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

static s32 drawParagraph(s32 tile, s32 panelX, s32 panelW, s32 y, s32 height, const char *text)
{
    char *remaining = (char *)text;
    font_setStyle(FONT_STYLE_8PX_B, COLOR_NORMAL, GAUNTLET_UI_TEXT_BACKGROUND);
    font_setLineHeight(CELL);
    return font_drawLinesInBox(tile, (panelX + 1) * CELL + 4, y, (panelW - 2) * CELL - 8, height,
                               &remaining, 0);
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
        if (gauntlet_roomType() == DOOR_TREASURE)
        {
            appendText(buffer, "TREASURE");
        }
        else
        {
            out = appendText(buffer, "ROOM ");
            out = appendNumber(out, gauntlet_roomsCleared());
            appendText(out, " CLEARED");
        }
        tile = drawTitle(tile, buffer);
        out = appendText(buffer, (ARG_OFFERS & REWARD_FALLBACK)
                                     ? (gauntlet_rewardRarity() == RARITY_EPIC ? "Epic pool empty"
                                                                               : "Rare pool empty")
                                     : "Choose a boon");
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
    font_setStyle(FONT_STYLE_8PX_B, COLOR_NORMAL, GAUNTLET_UI_TEXT_BACKGROUND);
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
    font_setStyle(FONT_STYLE_11PX_A, COLOR_YELLOW, GAUNTLET_UI_TEXT_BACKGROUND);
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

// Door cards: a framed card per door with the stock bonus-map icon art on it, a cursor frame around
// the selected icon, and a line of detail underneath.
#define CARD_W 9
#define CARDS_X FULL_X
#define CARDS_Y 2
#define CARDS_H 13
#define DOOR_DETAIL_Y 15
#define DOOR_DETAIL_H 5
#define DOOR_LABEL_Y 24
#define DOOR_ICON_Y 32
#define DOOR_ICON_WIDTH 32

static const char *const DoorTypeNames[DOOR_TYPE_COUNT] = {
    "Battle", "Elite", "Rest", "Treasure", "Skirmish", "Onslaught", "Shrine", "Trial"};
static const u8 DoorColors[DOOR_TYPE_COUNT] = {COLOR_WHITE, COLOR_RED, COLOR_GREEN,  COLOR_YELLOW,
                                               COLOR_GREEN, COLOR_RED, COLOR_YELLOW, COLOR_RED};

static GauntletDoor doorAt(u32 slot)
{
    u32 packed = slot < 2 ? ARG_DOORS_LOW >> (slot * 16) : ARG_DOORS_HIGH;

    return gauntlet_unpackDoor(packed & 0xffff);
}

static void setDoors(const GauntletDoor doors[GAUNTLET_DOOR_COUNT])
{
    ARG_DOORS_LOW = gauntlet_packDoor(&doors[0]) | gauntlet_packDoor(&doors[1]) << 16;
    ARG_DOORS_HIGH = gauntlet_packDoor(&doors[2]);
}

/** The stock level icon that shows a door: a landmark of its foes, or of the room's kind. */
static const SpriteAnimation *doorIcon(const GauntletDoor *door)
{
    switch (door->type)
    {
    case DOOR_TRIAL:
    case DOOR_ELITE:
        return &MultiplayerLevelSelectEntries[0].animation;
    case DOOR_ONSLAUGHT:
        return &BonusMapMenuEntries[2].animation;
    case DOOR_SHRINE:
        return &BonusMapMenuEntries[4].animation;
    case DOOR_REST:
        return &BonusMapMenuEntries[1].animation;
    case DOOR_TREASURE:
        return &BonusMapMenuEntries[4].animation;
    }
    switch (door->theme)
    {
    case THEME_GOBLINS:
        return &BonusMapMenuEntries[0].animation;
    case THEME_ORCS:
        return &MultiplayerLevelSelectEntries[3].animation;
    case THEME_URUKS:
        return &BonusMapMenuEntries[2].animation;
    case THEME_SPIDERS:
        return &BonusMapMenuEntries[3].animation;
    case THEME_SOUTH:
        return &MultiplayerLevelSelectEntries[2].animation;
    case THEME_WARGS:
        return &BonusMapMenuEntries[1].animation;
    default:
        return &MultiplayerLevelSelectEntries[1].animation;
    }
}

static s32 cardCenter(u32 slot)
{
    return ((CARDS_X + slot * CARD_W) * 2 + CARD_W) * CELL / 2;
}

static void placeDoorCursor(void)
{
    sprite_setPosition(g_BonusMapCursorObj,
                       (cardCenter(ARG_CURSOR) - DOOR_ICON_WIDTH / 2 - 1) << 16,
                       (DOOR_ICON_Y - 1) << 16);
}

/** Remove every sprite of the page: the door icons and the cursor frame. */
static void clearSprites(void)
{
    Actor *actor;

    for (actor = g_ActorListMain.head; actor != NULL; actor = actor->next)
    {
        actor->flags.p |= ACTOR_FLAG_PENDING_REMOVE;
    }
}

static void spawnDoorSprites(void)
{
    u32 slot;

    for (slot = 0; slot < GAUNTLET_DOOR_COUNT; slot++)
    {
        GauntletDoor door = doorAt(slot);
        Actor *icon = actor_allocMain();

        sprite_setPosition(icon, (cardCenter(slot) - DOOR_ICON_WIDTH / 2) << 16, DOOR_ICON_Y << 16);
        icon->flags.p = 1;
        icon->oam.objMode = 0;
        icon->oam.colors256 = 0;
        icon->oam.bgPriority = 1;
        icon->oam.paletteNumber = 3 + slot;
        icon->type = ACTOR_TYPE_UNKNOWN_00;
        sprite_setAnimation(icon, doorIcon(&door));
    }
    g_BonusMapCursorObj = actor_allocMain();
    g_BonusMapCursorObj->flags.p = 0x11;
    g_BonusMapCursorObj->oam.objMode = 0;
    g_BonusMapCursorObj->oam.colors256 = 0;
    g_BonusMapCursorObj->oam.paletteNumber = 0;
    g_BonusMapCursorObj->type = ACTOR_TYPE_UNKNOWN_00;
    sprite_setAnimation(g_BonusMapCursorObj, &BonusMapCursorAnimation);
    placeDoorCursor();
}

static const char *doorDetail(const GauntletDoor *door, char *buffer)
{
    char *out = buffer;

    switch (door->type)
    {
    case DOOR_REST:
        return gauntlet_hasHerbs()
                   ? "No fight. Gather 3 fresh herbs for Herbal Healing. Pouch limit: 10."
                   : "No fight. Gain 1 boon reroll. This hero cannot use healing herbs.";
    case DOOR_SHRINE:
        return "No fight. Gain random equipment, scaled to run depth. "
               "Equipped or stored in your pack. Full pack: 1 boon reroll instead.";
    case DOOR_TREASURE:
        return "A hoard left by the fallen. Take a rare boon and go on without a fight.";
    default:
        out = appendText(out, gauntlet_doorMapName(door));
        out = appendText(out, door->type == DOOR_ELITE ? ": champions of the " : ": ");
        out = appendText(out, gauntlet_doorThemeName(door->theme));
        out = appendText(out, door->type == DOOR_TRIAL
                                  ? ". Two champion waves, +2 tiers. No clear heal."
                              : door->type == DOOR_SKIRMISH  ? ". Two waves."
                              : door->type == DOOR_ONSLAUGHT ? ". Three ambush waves."
                                                             : ". Three waves.");
        if (door->modifier == ROOM_MODIFIER_ARMORED)
        {
            out = appendText(out, " +25% enemy HP.");
        }
        else if (door->modifier == ROOM_MODIFIER_RUSH)
        {
            out = appendText(out, " Wave breaks 1/3 as long.");
        }
        else if (door->modifier == ROOM_MODIFIER_SUPPLIES)
        {
            out = appendText(out, gauntlet_hasHerbs() ? " Clear: +2 herbs." : " Clear: +25% mana.");
        }
        out = appendText(out,
                         gauntlet_doorRewardRarity(door) == RARITY_EPIC   ? " Epic boon priority."
                         : gauntlet_doorRewardRarity(door) == RARITY_RARE ? " Rare boon priority."
                                                                          : " Win a boon.");
        if (door->type == DOOR_BATTLE)
        {
            appendText(out, " +1 boon reroll.");
        }
        return buffer;
    }
}

static const char *doorRewardLabel(const GauntletDoor *door)
{
    if (door->type == DOOR_REST)
    {
        return gauntlet_hasHerbs() ? "3 herbs" : "1 reroll";
    }
    if (door->type == DOOR_SHRINE)
    {
        return "Equipment";
    }
    u32 rarity = gauntlet_doorRewardRarity(door);
    if (door->type == DOOR_BATTLE)
    {
        return rarity == RARITY_RARE ? "Rare +R" : "Boon +R";
    }
    return rarity == RARITY_EPIC ? "Epic boon" : rarity == RARITY_RARE ? "Rare boon" : "Boon";
}

static void drawDoors(s32 tile)
{
    char buffer[160];
    u32 slot;

    tile = drawText(tile, COLOR_NORMAL, 12, TITLE_Y, "CHOOSE A DOOR", 0);
    appendNumber(appendText(buffer, "Room "), gauntlet_roomsCleared() + 1);
    tile = drawText(tile, COLOR_WHITE, 228, 5, buffer, 2);
    tile = drawText(tile, COLOR_WHITE, 139, 5, "Select: Shop", 1);
    gauntletUi_panel(FULL_X, DOOR_DETAIL_Y, FULL_W, DOOR_DETAIL_H);
    for (slot = 0; slot < GAUNTLET_DOOR_COUNT; slot++)
    {
        GauntletDoor door = doorAt(slot);

        gauntletUi_panel(CARDS_X + slot * CARD_W, CARDS_Y, CARD_W, CARDS_H);
        tile = drawText(tile, slot == ARG_CURSOR ? COLOR_YELLOW : DoorColors[door.type],
                        cardCenter(slot), DOOR_LABEL_Y, DoorTypeNames[door.type], 1);
        tile = drawText(tile,
                        gauntlet_doorHasCombat(door.type) || door.type == DOOR_TREASURE
                            ? RarityColors[gauntlet_doorRewardRarity(&door)]
                            : COLOR_GREEN,
                        cardCenter(slot), 96, doorRewardLabel(&door), 1);
        tile = drawText(tile, COLOR_WHITE, cardCenter(slot), 104,
                        gauntlet_modifierName(door.modifier), 1);
    }
    {
        GauntletDoor door = doorAt(ARG_CURSOR);

        drawParagraph(tile, FULL_X, FULL_W, (DOOR_DETAIL_Y + 1) * CELL, 3 * CELL,
                      doorDetail(&door, buffer));
    }
}

static void drawBriefing(s32 tile)
{
    char buffer[160];
    GauntletDoor door = gauntlet_roomDoor();
    char *out = appendText(buffer, "ROOM ");
    appendNumber(out, gauntlet_roomsCleared() + 1);
    tile = drawTitle(tile, buffer);
    gauntletUi_panel(FULL_X, OFFERS_Y, FULL_W, 14);
    tile = drawText(tile, DoorColors[door.type], CENTER_X, 40, gauntlet_roomName(), 1);
    tile = drawText(tile, COLOR_WHITE, CENTER_X, 64, gauntlet_encounterName(), 1);
    tile = drawParagraph(tile, FULL_X, FULL_W, 80, 5 * CELL, doorDetail(&door, buffer));
    gauntletUi_highlight(FULL_X + 1, 16, FULL_W - 2);
    drawText(tile, COLOR_WHITE, CENTER_X, 128, "A: Enter room", 1);
}

static bool isShopPage(u32 page)
{
    return page == GAUNTLET_PAGE_SHOP || page == GAUNTLET_PAGE_SHOP_ITEM ||
           page == GAUNTLET_PAGE_SHOP_SELL || page == GAUNTLET_PAGE_SHOP_SELL_ITEM;
}

static Item shopListItem(bool selling, u32 index)
{
    if (selling)
    {
        return index < 8 ? PLAYER(0).inventory.array[INVENTORY_SLOT_BACKPACK_0 + index]
                         : EMPTY_ITEM;
    }
    const GauntletShopOffer *offer = gauntletShop_offer(index);
    return offer != NULL ? offer->item : EMPTY_ITEM;
}

static s32 shopItemColor(Item item)
{
    return item.d.itemType == ITEM_TYPE_UNIQUE ? COLOR_YELLOW
           : item.d.itemType != ITEM_TYPE_ARROW && (ITEM_HAS_PREFIX(item) || ITEM_HAS_SUFFIX(item))
               ? COLOR_GREEN : COLOR_WHITE;
}

static void shopShortName(char buffer[200], Item item)
{
    const char *name = item_affix_formatName(buffer, item, 180);
    u32 length = 0;
    while (length < 196 && name[length] != '\0')
    {
        buffer[length] = name[length];
        length++;
    }
    buffer[length] = '\0';
    if (length != 0 && buffer[length - 1] == '@')
    {
        buffer[--length] = '\0';
    }
    if (font_getStringWidth(buffer) <= GAUNTLET_UI_SHOP_NAME_WIDTH)
    {
        return;
    }
    do
    {
        length -= length >= 2 && buffer[length - 2] == '@' ? 2 : 1;
        buffer[length] = '.';
        buffer[length + 1] = '.';
        buffer[length + 2] = '.';
        buffer[length + 3] = '\0';
        if (font_getStringWidth(buffer) <= GAUNTLET_UI_SHOP_NAME_WIDTH)
        {
            return;
        }
        buffer[length] = '\0';
    } while (length != 0);
}

static void shopIcon(Item item, s32 x, s32 y, u32 palette)
{
    if (item.d.itemType == ITEM_TYPE_EMPTY)
    {
        return;
    }
    Actor *icon = actor_allocMain();
    if (icon == NULL)
    {
        return;
    }
    sprite_setPosition(icon, x << 16, y << 16);
    icon->flags.p = 1;
    icon->oam.objMode = 0;
    icon->oam.colors256 = 0;
    icon->oam.bgPriority = 0;
    icon->oam.paletteNumber = palette;
    icon->type = ACTOR_TYPE_UNKNOWN_00;
    sprite_setAnimation(icon, &ITEM_TYPE_INFO(item).baseGfx[item.d.baseIndex].animation);
}

static Item shopComparedItem(Item item)
{
    if (item.d.itemType == ITEM_TYPE_EMPTY)
    {
        return EMPTY_ITEM;
    }
    u32 slot = item.d.itemType <= ITEM_TYPE_BOW ? INVENTORY_SLOT_MAINHAND
                                               : item_getEquipSlot(item.word, 0);
    return slot < INVENTORY_SLOT_BACKPACK_0 ? PLAYER(0).inventory.array[slot] : EMPTY_ITEM;
}

static void spawnShopIcons(void)
{
    bool selling = ARG_PAGE == GAUNTLET_PAGE_SHOP_SELL ||
                   ARG_PAGE == GAUNTLET_PAGE_SHOP_SELL_ITEM;
    if (ARG_PAGE == GAUNTLET_PAGE_SHOP || ARG_PAGE == GAUNTLET_PAGE_SHOP_SELL)
    {
        u32 first = selling ? ARG_CURSOR / 4 * 4 : 0;
        for (u32 row = 0; row < GAUNTLET_SHOP_SIZE; row++)
        {
            shopIcon(shopListItem(selling, first + row), 12, 40 + row * 24, 3 + row);
        }
    }
    else
    {
        Item item = selling ? gauntletShop_saleItem() : shopListItem(FALSE, ARG_CURSOR);
        shopIcon(item, 48, 48, 3);
        shopIcon(shopComparedItem(item), 160, 48, 4);
    }
}

static s32 drawShopHeader(s32 tile, const char *title)
{
    char balance[16];
    font_setStyle(FONT_STYLE_11PX_A, COLOR_YELLOW, GAUNTLET_UI_TEXT_BACKGROUND);
    tile = font_drawString(tile, 12, 5, (char *)title, 0);
    gauntletUi_panel(22, 0, 7, 3);
    gauntletUi_gem(23, 1);
    appendNumber(balance, PLAYER(0).gems);
    return drawText(tile, COLOR_WHITE, 222, 8, balance, 2);
}

static s32 drawShopFooter(s32 tile, const char *action, const char *back, const char *hint)
{
    gauntletUi_panel(FULL_X, 17, FULL_W, 3);
    tile = drawText(tile, COLOR_GREEN, 20, 140, "[A]", 0);
    tile = drawText(tile, COLOR_WHITE, 38, 140, action, 0);
    tile = drawText(tile, COLOR_RED, 126, 140, "[B]", 0);
    tile = drawText(tile, COLOR_WHITE, 144, 140, back, 0);
    s32 color = PageState.shop.message >= 2 && PageState.shop.message != 5
                    ? COLOR_RED : ShopMessage != NULL ? COLOR_GREEN : COLOR_WHITE;
    return drawText(tile, color, CENTER_X, 148, ShopMessage != NULL ? ShopMessage : hint, 1);
}

static void drawShop(s32 tile)
{
    static const char *const Kinds[] = {"DAGGER", "SWORD", "GREATSWORD", "AXE", "MACE",
                                        "STAFF", "BOW", "ARROWS", "ARMOR", "SHIELD",
                                        "JEWELRY", "CLOAK", "GLOVES", "BOOTS", "RELIC"};
    bool selling = ARG_PAGE == GAUNTLET_PAGE_SHOP_SELL;
    u32 first = selling ? ARG_CURSOR / 4 * 4 : 0;
    char buffer[200];

    tile = drawShopHeader(tile, "MERCHANT");
    gauntletUi_card(1, 3, 14, 2, !selling);
    gauntletUi_card(15, 3, 14, 2, selling);
    tile = drawText(tile, selling ? COLOR_WHITE : COLOR_YELLOW, 64, 28,
                    selling ? "BUY" : "> BUY <", 1);
    tile = drawText(tile, selling ? COLOR_YELLOW : COLOR_WHITE, 176, 28,
                    selling ? "> SELL <" : "SELL", 1);
    for (u32 row = 0; row < GAUNTLET_SHOP_SIZE; row++)
    {
        u32 index = first + row;
        const GauntletShopOffer *offer = selling ? NULL : gauntletShop_offer(index);
        Item item = shopListItem(selling, index);
        u32 price = selling ? gauntletShop_salePrice(item) : offer != NULL ? offer->price : 0;
        bool sold = !selling && offer != NULL && offer->sold;
        s32 y = 40 + row * 24;

        gauntletUi_card(FULL_X, y / CELL, FULL_W, 3, index == ARG_CURSOR);
        if (item.d.itemType == ITEM_TYPE_EMPTY)
        {
            tile = drawText(tile, COLOR_WHITE, 20, y + 8, "--", 0);
            tile = drawText(tile, COLOR_WHITE, 48, y + 4,
                            selling ? "Empty slot" : "No compatible gear", 0);
        }
        else
        {
            memory_memset(buffer, 0, sizeof(buffer));
            shopShortName(buffer, item);
            tile = drawText(tile, sold ? COLOR_WHITE : index == ARG_CURSOR ? COLOR_YELLOW
                                                                      : shopItemColor(item),
                            48, y + 4, buffer, 0);
            appendText(buffer, Kinds[item.d.itemType]);
            tile = drawText(tile, COLOR_WHITE, 48, y + 12, buffer, 0);
        }
        if (item.d.itemType != ITEM_TYPE_EMPTY)
        {
            char *out = appendNumber(buffer, price);
            appendText(out, "g");
            tile = drawText(tile, sold || (!selling && price > PLAYER(0).gems)
                                      ? COLOR_RED : COLOR_GREEN,
                            222, y + 4, sold ? "SOLD" : buffer, 2);
        }
        if (selling)
        {
            appendNumber(appendText(buffer, "Slot "), index + 1);
        }
        else
        {
            if (sold || item.d.itemType == ITEM_TYPE_EMPTY)
            {
                appendText(buffer, sold ? "TAKEN" : "--");
            }
            else
            {
                appendNumber(appendText(buffer, "Lv "), (u8)item_getMinLevel(item.word));
            }
        }
        tile = drawText(tile, COLOR_WHITE, 222, y + 12, buffer, 2);
    }
    drawShopFooter(tile, "Inspect", "Doors", selling
        ? (first == 0 ? "L/R: tabs  Up/Down: pack 1-4/8" : "L/R: tabs  Up/Down: pack 5-8/8")
        : "L/R: tabs   Up/Down: choose");
}

static s32 drawShopColumn(s32 tile, Item item, u32 column)
{
    char buffer[200];
    s32 x = column == 0 ? 1 : 15;
    char *remaining;

    gauntletUi_card(x, 4, 14, 13, column == 0);
    tile = drawText(tile, COLOR_WHITE, (x + 7) * CELL, 40,
                    column == 0 ? (ARG_PAGE == GAUNTLET_PAGE_SHOP_SELL_ITEM ? "SELL ITEM" : "NEW GEAR")
                                : "EQUIPPED", 1);
    ShopMore[column] = FALSE;
    ShopStatNext[column] = 0;
    if (item.d.itemType == ITEM_TYPE_EMPTY)
    {
        return drawText(tile, COLOR_WHITE, (x + 7) * CELL, 80, "None", 1);
    }
    memory_memset(buffer, 0, sizeof(buffer));
    remaining = item_affix_formatName(buffer, item, 180);
    font_setStyle(FONT_STYLE_8PX_B, shopItemColor(item), GAUNTLET_UI_TEXT_BACKGROUND);
    font_setLineHeight(CELL);
    tile = font_drawLinesInBox(tile, (x + 1) * CELL + 2, 76, 92, 16, &remaining, 0);
    memory_memset(buffer, 0, sizeof(buffer));
    item_affix_accumulateStats(buffer, item);
    remaining = buffer + ShopStatOffsets[column];
    font_setStyle(FONT_STYLE_8PX_B, COLOR_WHITE, GAUNTLET_UI_TEXT_BACKGROUND);
    tile = font_drawLinesInBox(tile, (x + 1) * CELL + 2, 96, 92, 32, &remaining, 0);
    ShopMore[column] = *remaining != '\0';
    ShopStatNext[column] = remaining - buffer;
    return tile;
}

static void drawShopItem(s32 tile)
{
    bool selling = ARG_PAGE == GAUNTLET_PAGE_SHOP_SELL_ITEM;
    const GauntletShopOffer *offer = selling ? NULL : gauntletShop_offer(ARG_CURSOR);
    Item item = selling ? gauntletShop_saleItem() : offer != NULL ? offer->item : EMPTY_ITEM;
    u32 price = selling ? gauntletShop_salePrice(item) : offer != NULL ? offer->price : 0;
    bool sold = !selling && offer != NULL && offer->sold;
    char buffer[80];
    char *out;

    if (item.d.itemType == ITEM_TYPE_EMPTY)
    {
        drawShop(tile);
        return;
    }
    tile = drawShopHeader(tile, selling ? "SELL GEAR?" : "INSPECT");
    out = appendText(buffer, selling ? "+" : "");
    if (sold)
    {
        out = appendText(out, "SOLD OUT");
    }
    else
    {
        out = appendNumber(out, price);
        out = appendText(out, " gems");
    }
    out = appendText(out, "  /  Lv ");
    out = appendNumber(out, (u8)item_getMinLevel(item.word));
    if (!sold && ((selling && PLAYER(0).gems + price <= 65535) || (!selling && PLAYER(0).gems >= price)))
    {
        out = appendText(out, "  /  After ");
        appendNumber(out, selling ? PLAYER(0).gems + price : PLAYER(0).gems - price);
    }
    tile = drawText(tile, sold || (!selling && PLAYER(0).gems < price) ? COLOR_RED : COLOR_GREEN,
                    CENTER_X, 24, buffer, 1);
    tile = drawShopColumn(tile, item, 0);
    tile = drawShopColumn(tile, shopComparedItem(item), 1);
    drawShopFooter(tile, selling ? "Confirm sale" : sold ? "Sold out" : "Buy item", selling ? "Cancel" : "Back",
                    "Up/Down: browse   L/R: stats");
}

static void drawScreen(void)
{
    if (isShopPage(ARG_PAGE))
    {
        clearSprites();
    }
    gauntletUi_beginFrame();
    switch (ARG_PAGE)
    {
    case GAUNTLET_PAGE_BRIEFING:
        drawBriefing(2);
        break;
    case GAUNTLET_PAGE_DOORS:
        drawDoors(2);
        break;
    case GAUNTLET_PAGE_SHOP:
    case GAUNTLET_PAGE_SHOP_SELL:
        drawShop(2);
        break;
    case GAUNTLET_PAGE_SHOP_ITEM:
    case GAUNTLET_PAGE_SHOP_SELL_ITEM:
        drawShopItem(2);
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
    if (isShopPage(ARG_PAGE))
    {
        spawnShopIcons();
    }
}

static void showPage(u32 page)
{
    if (ARG_PAGE == GAUNTLET_PAGE_DOORS && page != GAUNTLET_PAGE_DOORS)
    {
        clearSprites();
    }
    ARG_PAGE = page;
    ARG_CURSOR = 0;
    DetailStart = DetailNext = NULL;
    drawScreen();
}

/** Offer the doors to the next room. */
static void openDoors(void)
{
    GauntletDoor doors[GAUNTLET_DOOR_COUNT];

    gauntlet_rollDoors(doors);
    setDoors(doors);
    clearSprites();
    showPage(GAUNTLET_PAGE_DOORS);
    spawnDoorSprites();
}

/** Show the next page between rooms: queued level-up rewards, then the doors. */
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
    openDoors();
}

/** Roll the boon page's offers. FALSE when no boon is left to offer. */
static bool32 rollBoons(void)
{
    u8 offers[GAUNTLET_OFFER_COUNT];

    u8 preferred = gauntlet_rewardRarity();
    u8 actual = gauntletBoons_rollBestAvailable(offers, preferred);
    setOffers(offers);
    if (actual < preferred)
    {
        ARG_OFFERS |= REWARD_FALLBACK;
    }
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
    if (!isShopPage(ARG_PAGE))
    {
        DetailStart = DetailNext = NULL;
    }
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

static void tickDoors(u16 keys)
{
    if (keys & SELECT_BUTTON)
    {
        u32 selected = ARG_CURSOR;
        gauntletShop_open();
        sfx_play(SFX_MENU_CONFIRM);
        showPage(GAUNTLET_PAGE_SHOP);
        ShopDoorCursor = selected;
    }
    else if (keys & (DPAD_LEFT | DPAD_RIGHT))
    {
        moveCursor(keys & DPAD_LEFT ? -1 : 1, GAUNTLET_DOOR_COUNT, FALSE);
        placeDoorCursor();
    }
    else if (keys & A_BUTTON)
    {
        GauntletDoor door = doorAt(ARG_CURSOR);

        sfx_play(SFX_MENU_CONFIRM);
        switch (gauntlet_takeDoor(&door))
        {
        case DOOR_RESULT_ROOM:
            showPage(GAUNTLET_PAGE_BRIEFING);
            break;
        case DOOR_RESULT_BOON:
            if (rollBoons())
            {
                showPage(GAUNTLET_PAGE_BOON);
            }
            else
            {
                openDoors();
            }
            break;
        default:
            openDoors();
            break;
        }
    }
}

static bool inspectShopSelection(bool selling, u32 index)
{
    const GauntletShopOffer *offer = selling ? NULL : gauntletShop_offer(index);
    if (selling ? gauntletShop_prepareSale(INVENTORY_SLOT_BACKPACK_0 + index)
                : offer != NULL && offer->item.d.itemType != ITEM_TYPE_EMPTY)
    {
        ARG_CURSOR = index;
        ShopStatOffsets[0] = ShopStatOffsets[1] = 0;
        PageState.shop.message = 0;
        ARG_PAGE = selling ? GAUNTLET_PAGE_SHOP_SELL_ITEM : GAUNTLET_PAGE_SHOP_ITEM;
        drawScreen();
        return TRUE;
    }
    return FALSE;
}

static void tickShop(u16 keys)
{
    bool selling = ARG_PAGE == GAUNTLET_PAGE_SHOP_SELL;
    if (keys & (SELECT_BUTTON | DPAD_LEFT | DPAD_RIGHT | L_BUTTON | R_BUTTON))
    {
        bool target = keys & SELECT_BUTTON ? !selling : !!(keys & (DPAD_RIGHT | R_BUTTON));
        if (target != selling)
        {
            ARG_PAGE = target ? GAUNTLET_PAGE_SHOP_SELL : GAUNTLET_PAGE_SHOP;
            ARG_CURSOR = 0;
            PageState.shop.message = 0;
            drawScreen();
            sfx_play(SFX_MENU_CURSOR_MOVE);
        }
    }
    else if (keys & B_BUTTON)
    {
        clearSprites();
        ARG_PAGE = GAUNTLET_PAGE_DOORS;
        ARG_CURSOR = ShopDoorCursor;
        drawScreen();
        spawnDoorSprites();
        sfx_play(SFX_MENU_BACK);
    }
    else if (keys & (DPAD_UP | DPAD_DOWN))
    {
        PageState.shop.message = 0;
        moveCursor(keys & DPAD_UP ? -1 : 1, selling ? 8 : GAUNTLET_SHOP_SIZE, FALSE);
    }
    else if (keys & A_BUTTON)
    {
        sfx_play(inspectShopSelection(selling, ARG_CURSOR) ? SFX_MENU_CONFIRM : SFX_MENU_ERROR);
    }
}

static void tickShopItem(u16 keys)
{
    bool selling = ARG_PAGE == GAUNTLET_PAGE_SHOP_SELL_ITEM;
    if (keys & B_BUTTON)
    {
        PageState.shop.message = 0;
        ARG_PAGE = selling ? GAUNTLET_PAGE_SHOP_SELL : GAUNTLET_PAGE_SHOP;
        drawScreen();
        sfx_play(SFX_MENU_BACK);
    }
    else if (keys & (DPAD_UP | DPAD_DOWN))
    {
        u32 count = selling ? 8 : GAUNTLET_SHOP_SIZE;
        s32 step = keys & DPAD_UP ? -1 : 1;
        for (u32 attempt = 1; attempt <= count; attempt++)
        {
            u32 index = (ARG_CURSOR + count + step * (s32)attempt) % count;
            const GauntletShopOffer *offer = selling ? NULL : gauntletShop_offer(index);
            if (!selling && (offer == NULL || offer->sold))
            {
                continue;
            }
            if (inspectShopSelection(selling, index))
            {
                sfx_play(SFX_MENU_CURSOR_MOVE);
                break;
            }
        }
    }
    else if (keys & A_BUTTON)
    {
        u32 result = selling ? gauntletShop_sell() : gauntletShop_buy(ARG_CURSOR);
        PageState.shop.message = result + 1;
        if (result == SHOP_BOUGHT || result == SHOP_SOLD)
        {
            ARG_PAGE = selling ? GAUNTLET_PAGE_SHOP_SELL : GAUNTLET_PAGE_SHOP;
        }
        sfx_play(result == SHOP_BOUGHT || result == SHOP_SOLD ? SFX_MENU_CONFIRM : SFX_MENU_ERROR);
        drawScreen();
    }
    else if (keys & (L_BUTTON | R_BUTTON | DPAD_LEFT | DPAD_RIGHT))
    {
        if ((keys & (L_BUTTON | DPAD_LEFT)) || ShopMore[0] || ShopMore[1])
        {
            for (u32 column = 0; column < 2; column++)
            {
                ShopStatOffsets[column] = keys & (L_BUTTON | DPAD_LEFT) ? 0 : ShopStatNext[column];
            }
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
        case GAUNTLET_PAGE_DOORS:
            tickDoors(keys);
            break;
        case GAUNTLET_PAGE_SHOP:
        case GAUNTLET_PAGE_SHOP_SELL:
            tickShop(keys);
            break;
        case GAUNTLET_PAGE_SHOP_ITEM:
        case GAUNTLET_PAGE_SHOP_SELL_ITEM:
            tickShopItem(keys);
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
