#include "patches/debug_cheats/effect_preview.h"
#include "game.h"
#include "lib/skill_fx.h"
#include "scene.h"
#include "variables.h"

static u8 selected;
static u8 placement;
static u8 request;

void effectPreview_playerTick(u8 playerIndex)
{
    if (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE)
    {
        request = 0;
        return;
    }
    if (!request || playerIndex != ACTIVE_PLAYER_INDEX || g_SceneCurrent.id != SCENE_ID_GAMEPLAY ||
        g_GameFlags.d.overlay)
    {
        return;
    }
    request = 0;
    Actor *actor = PLAYER(playerIndex).ownerActor;
    if (actor != NULL)
    {
        Vector2Fp16 position = {actor->coords.x + 48 * FP16_16_ONE, actor->coords.y};
        hackLib_playEffect(selected, actor, placement ? &position : NULL, NULL);
    }
}

#if HACK_mod_options
static const char *const Places[] = {"On player", "In front"};
static const char *const Actions[] = {"Idle", "Play"};
#define OPTION(name, field)                                                                        \
    static u8 get##name(void)                                                                      \
    {                                                                                              \
        return field;                                                                              \
    }                                                                                              \
    static void set##name(u8 value)                                                                \
    {                                                                                              \
        field = value;                                                                             \
    }
OPTION(Effect, selected)
OPTION(Place, placement)
OPTION(Play, request)

const struct ModOptionEntry EffectPreviewOptions[EFFECT_PREVIEW_OPTION_COUNT] = {
    {"Effect", HackLibVisualEffectNames, HACKLIB_VISUAL_EFFECT_COUNT, 0, getEffect, setEffect},
    {"Placement", Places, 2, 0, getPlace, setPlace},
    {"Preview on resume", Actions, 2, 0, getPlay, setPlay},
};
#endif
