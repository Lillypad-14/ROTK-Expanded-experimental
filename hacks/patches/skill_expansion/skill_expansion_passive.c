#include "patches/skill_expansion/skill_expansion.h"
#include "stats.h"

// Replacements for the passive skill level functions over all SKILL_EXPANSION_PASSIVE_SLOT_COUNT
// slots. They also fix the stock off-by-one in applyAllLevels and the temporary-level functions
// passing a skill id where a slot is expected.

u8 skillExpansion_findPassiveSlot(u8 playerIndex, u8 passiveSkillId)
{
    u32 slot;

    if (passiveSkillId == SKILL_EXPANSION_PASSIVE_ID_NONE)
    {
        return SKILL_EXPANSION_PASSIVE_SLOT_NONE;
    }
    for (slot = 0; slot < SKILL_EXPANSION_PASSIVE_SLOT_COUNT; slot++)
    {
        if (PLAYER(playerIndex).passiveSkillIds[slot] == passiveSkillId)
        {
            return slot;
        }
    }
    return SKILL_EXPANSION_PASSIVE_SLOT_NONE;
}

/** Applied level of a passive skill in the player's tree, 0 when absent. */
s32 skillExpansion_getPassiveLevel(u8 playerIndex, u8 passiveSkillId)
{
    u8 slot = skillExpansion_findPassiveSlot(playerIndex, passiveSkillId);

    if (slot == SKILL_EXPANSION_PASSIVE_SLOT_NONE)
    {
        return 0;
    }
    return *skillExpansion_passiveApplied(playerIndex, slot);
}

/**
 * Add `level` applied levels' worth of the slot's stat records.
 *
 * REPLACES (whole function): skill_passive_applyStatsUpToLevel.
 */
void skillExpansion_applyStatsUpToLevel(u8 playerIndex, u8 slot, s8 level)
{
    const PassiveSkill *skill = skillExpansion_getPassiveSkill(PLAYER(playerIndex).passiveSkillIds[slot]);
    s32 record;

    if (skill == NULL)
    {
        return;
    }
    for (record = 0; record < 3; record++)
    {
        u8 statIndex = skill->records[record].statIndex;
        s32 i;

        if (statIndex == STAT_NONE)
        {
            continue;
        }
        for (i = 0; i < level; i++)
        {
            skill_passive_addStat(playerIndex, statIndex, (u8)skill->records[record].perLevel[i]);
        }
    }
}

/**
 * Apply (sign 1) or remove (sign -1) the stat records of the slot's current applied level.
 *
 * REPLACES (whole function): skill_passive_applyLevelStats.
 */
void skillExpansion_applyLevelStats(u8 playerIndex, u8 slot, s8 sign)
{
    const PassiveSkill *skill = skillExpansion_getPassiveSkill(PLAYER(playerIndex).passiveSkillIds[slot]);
    u8 level = *skillExpansion_passiveApplied(playerIndex, slot);
    s32 record;

    if (skill == NULL || level == 0 || level > PASSIVE_SKILL_MAX_LEVEL)
    {
        return;
    }
    for (record = 0; record < 3; record++)
    {
        u8 statIndex = skill->records[record].statIndex;
        u8 value;

        if (statIndex == STAT_NONE)
        {
            continue;
        }
        value = skill->records[record].perLevel[level - 1];
        if (sign == -1)
        {
            skill_passive_subtractStat(playerIndex, statIndex, value);
        }
        else
        {
            skill_passive_addStat(playerIndex, statIndex, value);
        }
    }
}

/**
 * REPLACES (whole function): skill_passive_applyAllLevels.
 */
void skillExpansion_passiveApplyAllLevels(u8 playerIndex)
{
    u32 slot;

    for (slot = 0; slot < SKILL_EXPANSION_PASSIVE_SLOT_COUNT; slot++)
    {
        s8 level = *skillExpansion_passiveApplied(playerIndex, slot);

        if (level != 0)
        {
            skillExpansion_applyStatsUpToLevel(playerIndex, slot, level);
        }
    }
}

/**
 * REPLACES (whole function): skill_passive_applyLevels.
 */
bool skillExpansion_passiveApplyLevels(u8 playerIndex, u8 passiveSkillId, u8 count)
{
    u8 slot = skillExpansion_findPassiveSlot(playerIndex, passiveSkillId);
    s8 *applied;
    u32 i;

    if (slot == SKILL_EXPANSION_PASSIVE_SLOT_NONE)
    {
        return FALSE;
    }
    applied = skillExpansion_passiveApplied(playerIndex, slot);
    for (i = 0; i < count; i++)
    {
        if (*applied < PASSIVE_SKILL_MAX_LEVEL)
        {
            (*applied)++;
            skillExpansion_applyLevelStats(playerIndex, slot, 1);
        }
    }
    return TRUE;
}

/**
 * REPLACES (whole function): skill_passive_addLevels.
 */
bool skillExpansion_passiveAddLevels(u8 playerIndex, u8 passiveSkillId, u8 count)
{
    u8 slot = skillExpansion_findPassiveSlot(playerIndex, passiveSkillId);
    const PassiveSkill *skill = skillExpansion_getPassiveSkill(passiveSkillId);
    s8 *purchased;

    if (slot == SKILL_EXPANSION_PASSIVE_SLOT_NONE || skill == NULL)
    {
        return FALSE;
    }
    purchased = skillExpansion_passivePurchased(playerIndex, slot);
    if (*purchased >= skill->maxLevel)
    {
        return FALSE;
    }

    *purchased += count;
    if (*purchased > PASSIVE_SKILL_MAX_LEVEL)
    {
        *purchased = PASSIVE_SKILL_MAX_LEVEL;
    }
    skillExpansion_passiveApplyLevels(playerIndex, passiveSkillId, count);

    if (passiveSkillId == PASSIVE_SKILL_FLEET_OF_FOOT)
    {
        PLAYER(playerIndex).combatFlags.p |= PLAYER_COMBAT_FLAG_MOVE_SPEED_DIRTY;
    }
    return TRUE;
}

/**
 * REPLACES (whole function): skill_passive_removeAppliedLevels.
 */
bool skillExpansion_passiveRemoveAppliedLevels(u8 playerIndex, u8 passiveSkillId, u8 count)
{
    u8 slot = skillExpansion_findPassiveSlot(playerIndex, passiveSkillId);
    s8 *applied;
    s8 *purchased;
    u32 i;

    if (slot == SKILL_EXPANSION_PASSIVE_SLOT_NONE)
    {
        return FALSE;
    }
    applied = skillExpansion_passiveApplied(playerIndex, slot);
    purchased = skillExpansion_passivePurchased(playerIndex, slot);
    for (i = 0; i < count; i++)
    {
        if (*applied > *purchased)
        {
            skillExpansion_applyLevelStats(playerIndex, slot, -1);
            (*applied)--;
        }
    }
    return TRUE;
}

/**
 * REPLACES (whole function): skill_passive_removeLevels.
 */
bool skillExpansion_passiveRemoveLevels(u8 playerIndex, u8 passiveSkillId, u8 count)
{
    u8 slot = skillExpansion_findPassiveSlot(playerIndex, passiveSkillId);
    s8 *purchased;

    if (slot == SKILL_EXPANSION_PASSIVE_SLOT_NONE)
    {
        return FALSE;
    }
    purchased = skillExpansion_passivePurchased(playerIndex, slot);
    *purchased -= count;
    if (*purchased < 0)
    {
        *purchased = 0;
    }
    else
    {
        skillExpansion_passiveRemoveAppliedLevels(playerIndex, passiveSkillId, count);
    }
    return TRUE;
}

/**
 * Add count temporary levels to every learned passive slot, capped at PASSIVE_SKILL_MAX_LEVEL.
 *
 * REPLACES (whole function): skill_passive_addTemporaryLevels.
 */
void skillExpansion_passiveAddTemporaryLevels(u8 playerIndex, u8 count)
{
    u32 n;
    u32 slot;

    for (n = 0; n < count; n++)
    {
        for (slot = 0; slot < SKILL_EXPANSION_PASSIVE_SLOT_COUNT; slot++)
        {
            s8 *applied = skillExpansion_passiveApplied(playerIndex, slot);

            if (*applied > 0 && *applied < PASSIVE_SKILL_MAX_LEVEL)
            {
                (*applied)++;
                skillExpansion_applyLevelStats(playerIndex, slot, 1);
            }
        }
    }
}

/**
 * Drop up to count temporary levels (applied above purchased) from every passive slot.
 *
 * REPLACES (whole function): skill_passive_removeTemporaryLevels.
 */
void skillExpansion_passiveRemoveTemporaryLevels(u8 playerIndex, u8 count)
{
    u32 n;
    u32 slot;

    for (n = 0; n < count; n++)
    {
        for (slot = 0; slot < SKILL_EXPANSION_PASSIVE_SLOT_COUNT; slot++)
        {
            s8 *applied = skillExpansion_passiveApplied(playerIndex, slot);

            if (*applied > *skillExpansion_passivePurchased(playerIndex, slot))
            {
                skillExpansion_applyLevelStats(playerIndex, slot, -1);
                (*applied)--;
            }
        }
    }
}
