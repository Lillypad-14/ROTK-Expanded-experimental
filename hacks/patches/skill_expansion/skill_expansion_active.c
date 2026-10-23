#include "game.h"
#include "input.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "patches/skill_expansion/skill_expansion_timeline.h"

// Replacements for the active skill level and cast-input functions over all
// SKILL_EXPANSION_ACTIVE_ROW_COUNT rows.

// L cycles through the rows in this order. Herbal Healing stays last.
static const u8 CycleOrder[] = {
    0, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, ACTIVE_SKILL_HERBAL_HEALING,
};

#define CYCLE_ORDER_COUNT (sizeof(CycleOrder) / sizeof(CycleOrder[0]))

static inline void revealSkillPanel(u8 playerIndex)
{
    g_PlayerHuds[playerIndex].skillPanelTimer = 0;
    g_PlayerHuds[playerIndex].skillPanelFrame = 0;
    g_PlayerHuds[playerIndex].skillPanelState = PLAYERHUD_SKILL_PANEL_REVEALING;
}

static inline bool rowHasCallback(u8 playerIndex, u32 row)
{
    return row < SKILL_EXPANSION_ACTIVE_ROW_COUNT &&
           PLAYER_ACTIVE_SKILL(playerIndex, row).callback != NULL;
}

/**
 * REPLACES (whole function): skill_active_applyLevels.
 */
void skillExpansion_activeApplyLevels(u8 playerIndex, u8 activeSkillIndex, u8 count)
{
    *skillExpansion_activeLevel(playerIndex, activeSkillIndex) += count;
}

/**
 * REPLACES (whole function): skill_active_addLevels.
 */
void skillExpansion_activeAddLevels(u8 playerIndex, u8 activeSkillIndex, u8 count)
{
    *skillExpansion_activePurchased(playerIndex, activeSkillIndex) += count;
    skillExpansion_activeApplyLevels(playerIndex, activeSkillIndex, count);
}

/**
 * REPLACES (whole function): skill_active_addPurchasedLevels.
 */
void skillExpansion_activeAddPurchasedLevels(u8 playerIndex, u8 activeSkillIndex, u8 count)
{
    *skillExpansion_activePurchased(playerIndex, activeSkillIndex) += count;
}

/**
 * REPLACES (whole function): skill_active_addTemporaryLevels.
 */
void skillExpansion_activeAddTemporaryLevels(u8 playerIndex, u8 activeSkillIndex, u8 count)
{
    *skillExpansion_activeLevel(playerIndex, activeSkillIndex) += count;
}

/**
 * REPLACES (whole function): skill_active_hasLevels.
 */
bool32 skillExpansion_activeHasLevels(u8 playerIndex, u8 activeSkillIndex)
{
    return *skillExpansion_activeLevel(playerIndex, activeSkillIndex) != 0;
}

static void addToAllRows(u8 playerIndex, u8 count, s32 delta)
{
    u32 n;
    u32 row;

    for (n = 0; n < count; n++)
    {
        for (row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
        {
            if (rowHasCallback(playerIndex, row))
            {
                *skillExpansion_activeLevel(playerIndex, row) += delta;
            }
        }
    }
}

/**
 * REPLACES (whole function): skill_active_addTemporaryLevelsToAll.
 */
void skillExpansion_activeAddTemporaryLevelsToAll(u8 playerIndex, u8 count)
{
    addToAllRows(playerIndex, count, 1);
}

/**
 * REPLACES (whole function): skill_active_removeTemporaryLevelsFromAll.
 */
void skillExpansion_activeRemoveTemporaryLevelsFromAll(u8 playerIndex, u8 count)
{
    addToAllRows(playerIndex, count, -1);
}

void skillExpansion_passiveAddTemporaryLevels(u8 playerIndex, u8 count);
void skillExpansion_passiveRemoveTemporaryLevels(u8 playerIndex, u8 count);

/**
 * REPLACES (whole function): skill_addTemporaryLevels.
 */
void skillExpansion_addTemporaryLevels(u8 playerIndex, u8 count)
{
    skillExpansion_passiveAddTemporaryLevels(playerIndex, count);
    addToAllRows(playerIndex, count, 1);
}

/**
 * REPLACES (whole function): skill_removeTemporaryLevels.
 */
void skillExpansion_removeTemporaryLevels(u8 playerIndex, u8 count)
{
    skillExpansion_passiveRemoveTemporaryLevels(playerIndex, count);
    addToAllRows(playerIndex, count, -1);
}

/** Stock per-hero cast requirements, keyed by the stock row a skill borrows them from. */
static bool32 meetsHeroRequirements(u8 playerIndex, Actor *actor, u32 ruleRow)
{
    ItemType offhandType = PLAYER(playerIndex).inventory.slots.offhand.d.itemType;
    ItemType mainHandType = PLAYER(playerIndex).inventory.slots.mainhand.d.itemType;

    switch (PLAYER(playerIndex).heroId)
    {
    case HERO_ID_FRODO:
    case HERO_ID_SAM:
        switch (ruleRow)
        {
        case ACTIVE_SKILL_FRODO_KNIFE_TOSS:
            return mainHandType != ITEM_TYPE_EMPTY;
        case ACTIVE_SKILL_FRODO_SNARE: {
            u8 level = PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_FRODO_SNARE];
            s16 maxSnares =
                skill_active_getLeveledValue(playerIndex, ACTIVE_SKILL_FRODO_SNARE, 1, level);
            return actor->as.combat.comboState[0] < maxSnares;
        }
        case ACTIVE_SKILL_FRODO_THE_ONE_RING:
        case ACTIVE_SKILL_FRODO_RINGS_PERSUASION:
            return PLAYER(playerIndex).heroId != HERO_ID_FRODO ||
                   g_CurrentMissionId != MISSION_CRACK_OF_DOOM_EDGE_OF_VOLCANO ||
                   g_MissionVariant != 7;
        }
        return TRUE;
    case HERO_ID_LEGOLAS:
        switch (ruleRow)
        {
        case ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD:
            return actor->as.combat.comboState[0] == 0;
        case ACTIVE_SKILL_LEGOLAS_SPREAD_FIRE:
            return mainHandType != ITEM_TYPE_EMPTY;
        case ACTIVE_SKILL_LEGOLAS_WHITE_KNIVES:
            return PLAYER(playerIndex).inventory.slots.backpack_0.d.itemType == ITEM_TYPE_KNIFE;
        }
        return TRUE;
    case HERO_ID_ARAGORN:
        switch (ruleRow)
        {
        case ACTIVE_SKILL_ARAGORN_SWEEP:
        case ACTIVE_SKILL_ARAGORN_SWORD_THROW:
            return mainHandType != ITEM_TYPE_EMPTY;
        case ACTIVE_SKILL_ARAGORN_CALL_OF_THE_DEAD:
            return actor->as.combat.comboState[0] == 0;
        }
        return TRUE;
    case HERO_ID_GANDALF:
        switch (ruleRow)
        {
        case ACTIVE_SKILL_GANDALF_SWORD_OF_POWER:
            // BUG: Unique swords have item type UNIQUE, so they cannot cast Sword of Power.
            return mainHandType <= ITEM_TYPE_SWORD_1H;
        case ACTIVE_SKILL_GANDALF_LIGHTSTRIKE:
            // BUG: Unique staves have item type UNIQUE, so they cannot cast Lightstrike.
            return offhandType == ITEM_TYPE_STAFF;
        }
        return TRUE;
    case HERO_ID_EOWYN:
        switch (ruleRow)
        {
        case ACTIVE_SKILL_EOWYN_DOUBLE_STRIKE:
            return mainHandType != ITEM_TYPE_EMPTY;
        case ACTIVE_SKILL_EOWYN_SHIELD_BASH:
            return offhandType == ITEM_TYPE_SHIELD;
        }
        return TRUE;
    case HERO_ID_GIMLI:
        switch (ruleRow)
        {
        case ACTIVE_SKILL_GIMLI_AXE_THROW:
        case ACTIVE_SKILL_GIMLI_WHIRLING_ATTACK:
            return mainHandType != ITEM_TYPE_EMPTY;
        }
        return TRUE;
    default:
        return TRUE;
    }
}

/**
 * Check whether a player can cast an active skill now. New rows use their base row's cooldown
 * bit and hero requirements.
 *
 * REPLACES (whole function): skill_active_canCast.
 */
bool32 skillExpansion_activeCanCast(u8 playerIndex, u8 activeSkillIndex)
{
    if (playerIndex >= 2 || activeSkillIndex >= SKILL_EXPANSION_ACTIVE_ROW_COUNT ||
        PLAYER(playerIndex).ownerActor == NULL || skillExpansion_isTimelineBusy(playerIndex))
    {
        return FALSE;
    }
    Actor *actor = PLAYER(playerIndex).ownerActor;
    u8 actionState = actor->actionState;
    u32 ruleRow = skillExpansion_baseRow(playerIndex, activeSkillIndex);
    const SkillExpansionActiveDef *def = skillExpansion_activeDef(playerIndex, activeSkillIndex);
    const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(PLAYER(playerIndex).heroId);
    const SkillExpansionCastPolicy *policy =
        skillExpansion_getCastPolicy(playerIndex, activeSkillIndex);
    u32 cooldownRow = policy != NULL ? policy->nativeCooldownRow : ruleRow;

    if (policy != NULL)
    {
        ruleRow = policy->requirementRow;
    }

    if (activeSkillIndex == ACTIVE_SKILL_HERBAL_HEALING && kit == NULL)
    {
        return PLAYER(playerIndex).kingsfoilHerbs.fresh != 0 ||
               PLAYER(playerIndex).kingsfoilHerbs.dried != 0;
    }
    if (!rowHasCallback(playerIndex, activeSkillIndex) ||
        (s8)*skillExpansion_activeLevel(playerIndex, activeSkillIndex) == 0)
    {
        return FALSE;
    }
    if (PLAYER(playerIndex).currentSpirit <
            PLAYER_ACTIVE_SKILL(playerIndex, activeSkillIndex).spiritCost &&
        !(g_GameFlags.p & GAME_FLAG_2))
    {
        return FALSE;
    }

    switch (actionState)
    {
    case ACTOR_STATE_LEGOLAS_OVERDRAW:
    case ACTOR_STATE_ACTIVE_SKILL_CAST_RECOVERY:
    case ACTOR_STATE_UNKNOWN_22:
    case ACTOR_STATE_ACTIVE_SKILL_CAST:
    case ACTOR_STATE_UNKNOWN_30:
    case ACTOR_STATE_KNOCKED_DOWN:
    case ACTOR_STATE_ACTIVE_SKILL_CAST_ANIMATION:
        return FALSE;
    default:
        break;
    }
    if (skillExpansion_runtime[playerIndex].cooldowns[activeSkillIndex] != 0 ||
        (cooldownRow < 32 && (actor->as.combat.actionStateFlags & (1u << cooldownRow))))
    {
        return FALSE;
    }
    if (def != NULL && def->canCast != NULL && !def->canCast(playerIndex))
    {
        return FALSE;
    }
    if (kit != NULL && kit->canCast != NULL && activeSkillIndex < HERO_ACTIVE_SKILL_COUNT &&
        !kit->canCast(playerIndex, activeSkillIndex))
    {
        return FALSE;
    }

    return ruleRow == SKILL_EXPANSION_RULE_NONE ||
           meetsHeroRequirements(playerIndex, actor, ruleRow);
}

static inline bool isCycleCandidate(u8 playerIndex, u32 row)
{
    return rowHasCallback(playerIndex, row) &&
           (s8)*skillExpansion_activeLevel(playerIndex, row) != 0 &&
           *skillExpansion_activeCycleEnabled(playerIndex, row) != 0;
}

static u32 cyclePosition(u32 row)
{
    u32 pos;

    for (pos = 0; pos < CYCLE_ORDER_COUNT; pos++)
    {
        if (CycleOrder[pos] == row)
        {
            return pos;
        }
    }
    return CYCLE_ORDER_COUNT;
}

/** The first cycle candidate at or after cycle position pos, or ACTIVE_SKILL_NONE. */
static u32 findEnabledSkill(u8 playerIndex, u32 pos)
{
    u32 attempts;

    for (attempts = 0; attempts < CYCLE_ORDER_COUNT; attempts++)
    {
        if (isCycleCandidate(playerIndex, CycleOrder[pos]))
        {
            return CycleOrder[pos];
        }
        pos = pos + 1 < CYCLE_ORDER_COUNT ? pos + 1 : 0;
    }
    return ACTIVE_SKILL_NONE;
}

/**
 * Cycle the selected active skill with L and cast it with A.
 *
 * REPLACES (whole function): skill_active_handleCycleCastModeInput.
 */
void skillExpansion_handleCycleCastModeInput(u8 playerIndex, bool fromHudInit)
{
    u32 selected = PLAYER(playerIndex).selectedActiveSkill.cycleSkillIndex;
    u8 hudState = g_PlayerHuds[playerIndex].skillPanelState;

    if (!fromHudInit && (PLAYER_KEYS_PRESSED(playerIndex) & A_BUTTON) != 0 &&
        (PLAYER(playerIndex).activeSkillStateFlags & ACTIVE_SKILL_STATE_FLAG_CASTING) == 0)
    {
        if (selected != ACTIVE_SKILL_NONE &&
            skillExpansion_activeCanCast(playerIndex, selected) == TRUE)
        {
            skill_active_cast(playerIndex, selected);
        }
        else if (selected != ACTIVE_SKILL_NONE)
        {
            skillExpansion_rejectCast(playerIndex, selected);
        }
        return;
    }

    if ((PLAYER_KEYS_PRESSED(playerIndex) & L_BUTTON) == 0 &&
        isCycleCandidate(playerIndex, selected))
    {
        return;
    }
    if (!fromHudInit && (hudState == PLAYERHUD_SKILL_PANEL_REVEALING ||
                         hudState == PLAYERHUD_SKILL_PANEL_RETRACTING))
    {
        return;
    }

    {
        u32 pos = cyclePosition(selected);

        pos = pos + 1 < CYCLE_ORDER_COUNT ? pos + 1 : 0;
        selected = findEnabledSkill(playerIndex, pos);
    }
    PLAYER(playerIndex).selectedActiveSkill.cycleSkillIndex = selected;
    if (selected == ACTIVE_SKILL_NONE)
    {
        return;
    }
    if (hudState == PLAYERHUD_SKILL_PANEL_IDLE)
    {
        revealSkillPanel(playerIndex);
    }
    else if (hudState == PLAYERHUD_SKILL_PANEL_SHOWING)
    {
        g_PlayerHuds[playerIndex].flags.p |= PLAYERHUD_FLAG_SKILL_PANEL_DIRTY;
    }
}

/**
 * Cast the active skill bound to L+A, L+B, or L+R and reveal its HUD panel.
 *
 * REPLACES (whole function): skill_active_handleQuickCastModeInput.
 */
void skillExpansion_handleQuickCastModeInput(u8 playerIndex)
{
    QuickCastSlot slot;
    u8 row;

    if ((PLAYER_KEYS_CURRENT(playerIndex) & L_BUTTON) == 0 ||
        (PLAYER(playerIndex).activeSkillStateFlags & ACTIVE_SKILL_STATE_FLAG_CASTING) != 0)
    {
        return;
    }

    if ((PLAYER_KEYS_PRESSED(playerIndex) & A_BUTTON) != 0)
    {
        slot = QUICK_CAST_SLOT_A;
    }
    else if ((PLAYER_KEYS_PRESSED(playerIndex) & B_BUTTON) != 0)
    {
        slot = QUICK_CAST_SLOT_B;
    }
    else if ((PLAYER_KEYS_PRESSED(playerIndex) & R_BUTTON) != 0)
    {
        slot = QUICK_CAST_SLOT_R;
    }
    else
    {
        return;
    }

    row = PLAYER(playerIndex).quickSelectActiveSkills[slot];
    if (skillExpansion_activeCanCast(playerIndex, row) == TRUE)
    {
        PLAYER(playerIndex).selectedActiveSkill.quickSlot = slot;
        skill_active_cast(playerIndex, row);
    }
    else if (row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT ||
             (s8)*skillExpansion_activeLevel(playerIndex, row) == 0)
    {
        return;
    }
    else
    {
        skillExpansion_rejectCast(playerIndex, row);
    }

    if (g_PlayerHuds[playerIndex].skillPanelState == PLAYERHUD_SKILL_PANEL_IDLE)
    {
        revealSkillPanel(playerIndex);
    }
    else if (g_PlayerHuds[playerIndex].skillPanelState == PLAYERHUD_SKILL_PANEL_SHOWING)
    {
        g_PlayerHuds[playerIndex].flags.p |= PLAYERHUD_FLAG_SKILL_PANEL_DIRTY;
    }
}
