#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "game.h"
#include "lib/actor_ref.h"
#include "lib/profile.h"
#include "lib/summon_registry.h"
#include "lib/timed_effect.h"
#include "memory.h"
#include "patches/skill_expansion/skill_delivery.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_frodo.h"
#include "patches/skill_expansion/skill_expansion_gimli.h"
#include "patches/skill_expansion/skill_expansion_legolas.h"
#include "patches/skill_expansion/skill_expansion_timeline.h"
#include "scene.h"
#include "stats.h"
#if HACK_twists
#include "patches/twists/twists.h"
#endif
#if HACK_debug_cheats
#include "patches/debug_cheats/ability_bench.h"
#include "patches/debug_cheats/debug_lab.h"
#include "patches/debug_cheats/delivery_preview.h"
#endif

SkillExpansionRuntime skillExpansion_runtime[2];

asm(".section .text\n"
    ".balign 4\n"
    ".thumb\n"
    ".global skillExpansion_original_player_tickHud\n"
    ".thumb_func\n"
    "skillExpansion_original_player_tickHud:\n"
    "    push {r4-r7, lr}\n"
    "    add r4, r0, #0\n"
    "    ldr r7, =g_PlayerHuds\n"
    "    lsl r0, r4, #5\n"
    "    ldr r3, =player_tickHud + 8\n"
    "    bx r3\n"
    "    .pool\n");

void skillExpansion_original_player_tickHud(s32 playerIndex);

void skillExpansion_prepareHud(u8 playerIndex)
{
    SkillExpansionRuntime *runtime;
    PlayerHud *hud;

    if (playerIndex >= 2 || playerIndex != ACTIVE_PLAYER_INDEX ||
        g_SceneCurrent.id != SCENE_ID_GAMEPLAY || g_GameFlags.d.overlay)
    {
        return;
    }
    runtime = &skillExpansion_runtime[playerIndex];
    hud = &g_PlayerHuds[playerIndex];
    // Native pickup and interaction text takes priority.
    if (hud->flags.p & PLAYERHUD_FLAG_0)
    {
        return;
    }
#if HACK_debug_cheats
    debugLab_drawTelemetry(playerIndex);
    deliveryPreview_drawTelemetry(playerIndex);
#endif
    if (runtime->feedbackFrames != 0)
    {
        hud->text = (char *)runtime->feedback;
        hud->textColor = HUD_TEXT_COLOR_GAMEPLAY_YELLOW;
        hud->flags.p |= PLAYERHUD_FLAG_0 | PLAYERHUD_FLAG_1;
    }
}

void skillExpansion_tickHud(s32 playerIndex)
{
    skillExpansion_prepareHud(playerIndex);
    skillExpansion_original_player_tickHud(playerIndex);
    if (playerIndex < 2 && playerIndex == ACTIVE_PLAYER_INDEX &&
        g_SceneCurrent.id == SCENE_ID_GAMEPLAY && !g_GameFlags.d.overlay)
    {
        const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(PLAYER(playerIndex).heroId);

        skillExpansion_frodoDrawBurdenBar(playerIndex);
        skillExpansion_aragornDrawFlameBar(playerIndex);
        skillExpansion_gimliDrawTallyBar(playerIndex);
        if (kit != NULL && kit->drawHud != NULL)
        {
            kit->drawHud(playerIndex);
        }
    }
}

void skillExpansion_feedback(u8 playerIndex, const char *message)
{
    skillExpansion_runtime[playerIndex].feedback = message;
    skillExpansion_runtime[playerIndex].feedbackFrames = 90;
}

void skillExpansion_runtimeBegin(u8 playerIndex, u8 heroId)
{
#if HACK_debug_cheats
    abilityBench_endSession();
#endif
    skillDelivery_clear(playerIndex);
    skillExpansion_clearTimeline(playerIndex);
    SkillExpansionRuntime *runtime = &skillExpansion_runtime[playerIndex];
    Actor *actor = PLAYER(playerIndex).ownerActor;
    Actor *cursor;
    bool ownerAlive = FALSE;

    hackLib_clearOwnerEffects(runtime->owner);
    hackLib_clearOwnerSummons(runtime->owner);

    for (cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
    {
        ownerAlive |= cursor == runtime->owner;
    }
    if (runtime->initialized && runtime->owner == actor && ownerAlive &&
        runtime->ownerGeneration == hackLib_actorGeneration(actor))
    {
        if (runtime->kit != NULL && runtime->kit->cleanup != NULL)
        {
            runtime->kit->cleanup(playerIndex, actor);
        }
        skillExpansion_eliteCleanup(playerIndex, actor);
        cursor = NULL;
        while ((cursor = hackLib_nextOwnedGhost(actor, cursor)) != NULL)
        {
            ghostState(cursor)->lifetime = 1;
        }
    }
    memory_memset(runtime, 0, sizeof(*runtime));
    runtime->owner = actor;
    runtime->ownerGeneration = hackLib_actorGeneration(actor);
    runtime->heroId = heroId;
    runtime->kit = skillExpansion_getHeroKit(heroId);
    runtime->initialized = TRUE;
    skillExpansion_eliteReset(playerIndex);
    if (runtime->kit != NULL && runtime->kit->reset != NULL)
    {
        runtime->kit->reset(playerIndex);
    }
#if HACK_twists
    twists_missionBegin(playerIndex);
#endif
}

void skillExpansion_runtimeInstall(u8 playerIndex)
{
    SkillExpansionRuntime *runtime = &skillExpansion_runtime[playerIndex];
    u32 row;

    for (row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        const SkillExpansionActiveDef *def = skillExpansion_activeDef(playerIndex, row);
        bool hasResolver = (def != NULL && (def->resolve != NULL || def->delivery != NULL)) ||
                           (runtime->kit != NULL && row < HERO_ACTIVE_SKILL_COUNT &&
                            runtime->kit->resolvers[row] != NULL);
        runtime->callbacks[row] = PLAYER_ACTIVE_SKILL(playerIndex, row).callback;
        if (runtime->callbacks[row] != NULL || hasResolver)
        {
            skillExpansion_players[playerIndex].activeSkills[row].callback =
                skillExpansion_completeCast;
        }
    }
}

void skillExpansion_failCast(u8 playerIndex, const char *message)
{
    SkillExpansionRuntime *runtime = &skillExpansion_runtime[playerIndex];
    s32 refund = runtime->pending ? runtime->paidSpirit : 0;
    s32 missing = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT) - PLAYER(playerIndex).currentSpirit;

    runtime->pending = FALSE;
    runtime->paidSpirit = 0;
    runtime->lastResult = SKILL_CAST_UNAVAILABLE;
    if (refund > missing)
    {
        refund = missing;
    }
    // Refund a paid resource directly. Hero gain restrictions do not apply to refunds.
    if (refund > 0)
    {
        if (runtime->kit != NULL && runtime->kit->refundSpirit != NULL)
        {
            runtime->kit->refundSpirit(playerIndex, refund);
        }
        else
        {
            PLAYER(playerIndex).currentSpirit += refund;
        }
    }
    skillExpansion_feedback(playerIndex, message);
}

void skillExpansion_finishCast(u8 playerIndex, SkillExpansionCastResult result)
{
    static const char *const Messages[] = {
        "Cast complete",        "No valid target",     "No safe landing",
        "Summon limit reached", "Ability unavailable", "Cast interrupted",
    };
    SkillExpansionRuntime *runtime = &skillExpansion_runtime[playerIndex];

    skillExpansion_gimliCastEnded(playerIndex, result == SKILL_CAST_SUCCESS);
    if (result != SKILL_CAST_SUCCESS)
    {
        skillExpansion_failCast(playerIndex, (u32)result < ARRAY_COUNT(Messages)
                                                 ? Messages[result]
                                                 : "Ability unavailable");
    }
    else
    {
        const SkillExpansionCastPolicy *policy =
            skillExpansion_getCastPolicy(playerIndex, runtime->pendingRow);
        if (runtime->pending && policy != NULL)
        {
            runtime->cooldowns[runtime->pendingRow] = policy->cooldownFrames;
        }
        runtime->pending = FALSE;
        runtime->paidSpirit = 0;
    }
    runtime->lastResult = result;
}

void skillExpansion_cast(u8 playerIndex, u8 row)
{
    SkillExpansionRuntime *runtime;
    Actor *actor;
    s32 before;

    if (playerIndex >= 2 || row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT ||
        !skillExpansion_activeCanCast(playerIndex, row))
    {
        if (playerIndex < 2)
        {
            skillExpansion_rejectCast(playerIndex, row);
        }
        return;
    }
    runtime = &skillExpansion_runtime[playerIndex];
    actor = PLAYER(playerIndex).ownerActor;
    if (runtime->pending)
    {
        return;
    }
    const HackLibCastTimeline *timeline = skillExpansion_getTimeline(playerIndex, row);
    if (timeline != NULL && !skillExpansion_prepareTimeline(playerIndex, actor, timeline))
    {
        skillExpansion_feedback(playerIndex, "Cast timeline unavailable");
        return;
    }
    runtime->timelineCast = timeline != NULL;
    before = PLAYER(playerIndex).currentSpirit;
    if (!(g_GameFlags.p & GAME_FLAG_2))
    {
        player_addSpirit(playerIndex, -(s32)PLAYER_ACTIVE_SKILL(playerIndex, row).spiritCost);
    }
    runtime->paidSpirit = before - PLAYER(playerIndex).currentSpirit;
    runtime->pendingRow = row;
    runtime->pending = TRUE;
    PLAYER(playerIndex).castingActiveSkill = row;
    skillExpansion_gimliCastStarted(playerIndex);
    if (row == ACTIVE_SKILL_HERBAL_HEALING)
    {
        skillExpansion_completeCast(actor);
    }
    else
    {
        actor->actionState = ACTOR_STATE_ACTIVE_SKILL_CAST;
        PLAYER(playerIndex).activeSkillStateFlags |= ACTIVE_SKILL_STATE_FLAG_CASTING;
    }
    skillExpansion_tickTimeline(playerIndex, actor);
}

void skillExpansion_completeCast(Actor *actor)
{
    u8 player = actor->as.combat.playerIndex;
    if (player < 2 && !skillExpansion_runtime[player].timelineCast)
    {
        skillExpansion_resolveTimelineCast(actor);
    }
}

void skillExpansion_resolveTimelineCast(Actor *actor)
{
    u8 p = actor->as.combat.playerIndex;
    if (p >= 2 || PLAYER(p).ownerActor != actor)
    {
        return;
    }
    u8 row = PLAYER(p).castingActiveSkill;
    SkillExpansionRuntime *runtime = &skillExpansion_runtime[p];
    const SkillExpansionActiveDef *def = skillExpansion_activeDef(p, row);
    SkillExpansionResolver resolver = def != NULL ? def->resolve : NULL;
    const SkillDeliveryDef *delivery = def != NULL ? def->delivery : NULL;

    if (resolver == NULL && runtime->kit != NULL && row < HERO_ACTIVE_SKILL_COUNT)
    {
        resolver = runtime->kit->resolvers[row];
    }

    if (row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT ||
        (runtime->callbacks[row] == NULL && resolver == NULL && delivery == NULL) ||
        (runtime->pending && runtime->pendingRow != row))
    {
        skillExpansion_finishCast(p, SKILL_CAST_UNAVAILABLE);
        return;
    }
    if ((def != NULL && def->canCast != NULL && !def->canCast(p)) ||
        (runtime->kit != NULL && runtime->kit->canCast != NULL && row < HERO_ACTIVE_SKILL_COUNT &&
         !runtime->kit->canCast(p, row)))
    {
        skillExpansion_finishCast(p, SKILL_CAST_INVALID_TARGET);
        return;
    }
    {
        HackLibDamageSource source = {actor, HACKLIB_DAMAGE_ABILITY, row,
                                      0,     HACKLIB_ELEMENT_NONE,   FALSE};
        const HackLibDamageSource *previous = hackLib_swapDamageSource(&source);

        runtime->lastResult = SKILL_CAST_SUCCESS;
        if (delivery != NULL)
        {
            bool launched =
                actor->facing < 8 &&
                skillDelivery_launch(p, delivery, &actor->coords, actor->facing * 4,
                                     skillExpansion_effectiveActiveLevel(p, row), row, FALSE);
            skillExpansion_finishCast(p, launched ? SKILL_CAST_SUCCESS : SKILL_CAST_UNAVAILABLE);
            if (launched && runtime->cooldowns[row] < delivery->cooldownTicks)
            {
                runtime->cooldowns[row] = delivery->cooldownTicks;
            }
        }
        else if (resolver != NULL)
        {
            skillExpansion_finishCast(p, resolver(actor));
        }
        else
        {
            runtime->callbacks[row](actor);
            if (runtime->pending)
            {
                skillExpansion_finishCast(p, runtime->lastResult);
            }
        }
        hackLib_swapDamageSource(previous);
    }
    runtime->pending = FALSE;
    runtime->paidSpirit = 0;
}

void skillExpansion_rejectCast(u8 playerIndex, u8 row)
{
    const char *message = "Ability unavailable";

    if (row < SKILL_EXPANSION_ACTIVE_ROW_COUNT &&
        PLAYER(playerIndex).currentSpirit < PLAYER_ACTIVE_SKILL(playerIndex, row).spiritCost &&
        !(g_GameFlags.p & GAME_FLAG_2))
    {
        message = "Not enough Spirit";
    }
    message = skillExpansion_legolasRejectMessage(playerIndex, row, message);
    skillExpansion_feedback(playerIndex, message);
}

void skillExpansion_runtimeTick(u8 playerIndex, Actor *actor)
{
    SkillExpansionRuntime *runtime = &skillExpansion_runtime[playerIndex];

    if (runtime->heroId != PLAYER(playerIndex).heroId)
    {
        skillExpansion_initHeroLoadout(PLAYER(playerIndex).heroId, playerIndex);
        runtime = &skillExpansion_runtime[playerIndex];
    }
    else if (runtime->owner != actor || runtime->ownerGeneration != hackLib_actorGeneration(actor))
    {
        skillDelivery_clear(playerIndex);
        skillExpansion_clearTimeline(playerIndex);
        hackLib_clearOwnerEffects(runtime->owner);
        hackLib_clearOwnerSummons(runtime->owner);
        runtime->owner = actor;
        runtime->ownerGeneration = hackLib_actorGeneration(actor);
        runtime->pending = FALSE;
        runtime->paidSpirit = 0;
        memory_memset(runtime->cooldowns, 0, sizeof(runtime->cooldowns));
        skillExpansion_eliteReset(playerIndex);
        if (runtime->kit != NULL && runtime->kit->reset != NULL)
        {
            runtime->kit->reset(playerIndex);
        }
    }
    for (u32 row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        if (runtime->cooldowns[row] != 0)
        {
            runtime->cooldowns[row]--;
        }
    }
    skillDelivery_tick(playerIndex, actor);
    skillExpansion_tickTimeline(playerIndex, actor);
    if (runtime->pending && !skillExpansion_isTimelineBusy(playerIndex) &&
        actor->actionState != ACTOR_STATE_ACTIVE_SKILL_CAST &&
        actor->actionState != ACTOR_STATE_ACTIVE_SKILL_CAST_ANIMATION &&
        !(PLAYER(playerIndex).activeSkillStateFlags & ACTIVE_SKILL_STATE_FLAG_CASTING))
    {
        skillExpansion_finishCast(playerIndex, SKILL_CAST_INTERRUPTED);
    }
    if (runtime->kit != NULL && runtime->kit->tick != NULL)
    {
        HackLibProfileSpan profile = hackLib_profileBegin(HACKLIB_PROFILE_HERO);
        runtime->kit->tick(playerIndex, actor);
        hackLib_profileEnd(profile);
    }
#if HACK_debug_cheats
    debugLab_playerTick(playerIndex);
#endif
    if (runtime->feedbackFrames != 0)
    {
        runtime->feedbackFrames--;
    }
}
