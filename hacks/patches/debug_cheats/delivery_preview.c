#include "patches/debug_cheats/delivery_preview.h"
#include "game.h"
#include "lib/combat_source.h"
#include "patches/skill_expansion/skill_delivery.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "scene.h"
#include "text.h"
#include "variables.h"

static u8 recipe, level, damageMode, placement, fireRequest, clearRequest, display;

void deliveryPreview_playerTick(u8 p)
{
    if (p != ACTIVE_PLAYER_INDEX)
    {
        return;
    }
    if ((g_GameFlags.p & GAME_FLAG_COOP_ACTIVE) || !(g_GameFlags.p & GAME_FLAG_BONUS_MAP_ACTIVE))
    {
        if (fireRequest || clearRequest)
        {
            skillExpansion_feedback(p, "Delivery test: bonus maps only");
        }
        fireRequest = clearRequest = display = 0;
        return;
    }
    if (g_SceneCurrent.id != SCENE_ID_GAMEPLAY || g_GameFlags.d.overlay)
    {
        return;
    }
    if (clearRequest)
    {
        clearRequest = 0;
        skillDelivery_clear(p);
    }
    if (!fireRequest)
    {
        return;
    }
    fireRequest = 0;
    Actor *owner = PLAYER(p).ownerActor;
    if (owner == NULL || recipe >= SKILL_DELIVERY_COUNT || owner->facing >= 8)
    {
        return;
    }
    Vector2Fp16 position = owner->coords;
    u8 direction = owner->facing * 4;
    if (placement)
    {
        position.x += Direction32MoveVectors[direction].x * 48;
        position.y += Direction32MoveVectors[direction].y * 48;
    }
    skillDelivery_resetStats(p);
    if (!skillDelivery_launch(p, &SkillDeliveryDefinitions[recipe], &position, direction, level + 1,
                              HACKLIB_ABILITY_NONE, !damageMode))
    {
        skillExpansion_feedback(p, "Delivery test unavailable");
    }
}

static char *number(char *out, u32 value)
{
    text_formatInt(value > 999999 ? 999999 : value, out);
    while (*out)
    {
        out++;
    }
    return out;
}

void deliveryPreview_drawTelemetry(u8 p)
{
    if (!display || p != ACTIVE_PLAYER_INDEX || (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE) ||
        !(g_GameFlags.p & GAME_FLAG_BONUS_MAP_ACTIVE))
    {
        return;
    }
    SkillDeliveryStats stats;
    skillDelivery_getStats(p, &stats);
    char *out = g_PlayerHuds[p].textBuffer;
    *out++ = 'C';
    *out++ = ':';
    out = number(out, stats.contacts);
    *out++ = ' ';
    *out++ = 'H';
    *out++ = ':';
    out = number(out, stats.hits);
    *out++ = ' ';
    *out++ = 'D';
    *out++ = ':';
    out = number(out, stats.damage);
    *out++ = ' ';
    *out++ = 'A';
    *out++ = ':';
    out = number(out, stats.active);
    *out = 0;
    g_PlayerHuds[p].text = g_PlayerHuds[p].textBuffer;
    g_PlayerHuds[p].textColor = HUD_TEXT_COLOR_GAMEPLAY_WHITE;
    g_PlayerHuds[p].flags.p |= PLAYERHUD_FLAG_0 | PLAYERHUD_FLAG_1;
}

#if HACK_mod_options
static const char *const Levels[] = {"1", "2", "3", "4", "5"};
static const char *const Modes[] = {"Visual only", "Deal damage"};
static const char *const Placements[] = {"On player", "In front"};
static const char *const Toggles[] = {"Off", "On"};
static const char *const Actions[] = {"Idle", "Run"};
#define OPTION(name, field)                                                                        \
    static u8 get##name(void)                                                                      \
    {                                                                                              \
        return field;                                                                              \
    }                                                                                              \
    static void set##name(u8 value)                                                                \
    {                                                                                              \
        field = value;                                                                             \
    }
OPTION(Recipe, recipe)
OPTION(Level, level)
OPTION(Damage, damageMode)
OPTION(Placement, placement)
OPTION(Fire, fireRequest)
OPTION(Clear, clearRequest)
OPTION(Display, display)
const struct ModOptionEntry DeliveryPreviewOptions[DELIVERY_PREVIEW_OPTION_COUNT] = {
    {"Recipe", SkillDeliveryNames, SKILL_DELIVERY_COUNT, 0, getRecipe, setRecipe, .desc = NULL},
    {"Test level", Levels, 5, 0, getLevel, setLevel, .desc = NULL},
    {"Mode", Modes, 2, 0, getDamage, setDamage, .desc = NULL},
    {"Placement", Placements, 2, 0, getPlacement, setPlacement, .desc = NULL},
    {"Fire on resume", Actions, 2, 0, getFire, setFire, .desc = NULL},
    {"Clear", Actions, 2, 0, getClear, setClear, .desc = NULL},
    {"Results", Toggles, 2, 0, getDisplay, setDisplay, .desc = NULL},
};
#endif
