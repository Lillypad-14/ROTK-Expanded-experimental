#pragma once

#include "patches/skill_expansion/skill_expansion.h"

typedef struct SkillExpansionRuntime
{
    Actor *owner;
    u32 ownerGeneration;
    const SkillExpansionHeroKit *kit;
    ActiveSkillCallback callbacks[SKILL_EXPANSION_ACTIVE_ROW_CAPACITY];
    u16 paidSpirit;
    u16 feedbackFrames;
    u8 pendingRow;
    bool pending;
    bool initialized;
    u8 heroId;
    const char *feedback;
    SkillExpansionCastResult lastResult;
    u16 cooldowns[SKILL_EXPANSION_ACTIVE_ROW_CAPACITY];
    bool timelineCast;
    bool feedbackDrawn;
} SkillExpansionRuntime;

extern SkillExpansionRuntime skillExpansion_runtime[2];

void skillExpansion_runtimeBegin(u8 playerIndex, u8 heroId);
void skillExpansion_runtimeInstall(u8 playerIndex);
void skillExpansion_runtimeTick(u8 playerIndex, Actor *actor);
void skillExpansion_cast(u8 playerIndex, u8 row);
void skillExpansion_completeCast(Actor *actor);
void skillExpansion_resolveTimelineCast(Actor *actor);
void skillExpansion_failCast(u8 playerIndex, const char *message);
void skillExpansion_finishCast(u8 playerIndex, SkillExpansionCastResult result);
void skillExpansion_feedback(u8 playerIndex, const char *message);
void skillExpansion_rejectCast(u8 playerIndex, u8 row);
void skillExpansion_prepareHud(u8 playerIndex);
void skillExpansion_tickHud(s32 playerIndex);
