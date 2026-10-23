#include "patches/skill_expansion/skill_expansion.h"
#include "game.h"
#include "memory.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "variables.h"

#if HACK_ringwraith
#include "hero_kit_declarations.h"
#include "patches/ringwraith/ringwraith_kit.h"
#endif

SkillExpansionPlayer skillExpansion_players[2];

static const ActiveSkill *const StockActiveSkills[SKILL_EXPANSION_HERO_COUNT] = {
    ActiveSkillsFrodo,   ActiveSkillsLegolas, ActiveSkillsAragorn,
    ActiveSkillsGandalf, ActiveSkillsEowyn,   ActiveSkillsGimli,
    ActiveSkillsSam,     ActiveSkillsSmeagol, ActiveSkillsAragorn,
};

static const u8 *const StockPassiveSkillIds[SKILL_EXPANSION_HERO_COUNT] = {
    PassiveSkillIdsFrodo,   PassiveSkillIdsLegolas, PassiveSkillIdsAragorn,
    PassiveSkillIdsGandalf, PassiveSkillIdsEowyn,   PassiveSkillIdsGimli,
    PassiveSkillIdsSam,     PassiveSkillIdsSmeagol, PassiveSkillIdsAragorn,
};

// Heroes past Smeagol fall back to Aragorn's block.
const u8 SkillExpansionHeroSkillBlocks[SKILL_EXPANSION_HERO_COUNT] = {
#include "hero_skill_blocks.inc"
};

#if HACK_ringwraith
#define HERO_KIT(name) (&name)
#else
#define HERO_KIT(name) NULL
#endif

static const SkillExpansionHeroKit *const HeroKits[SKILL_EXPANSION_HERO_COUNT] = {
#include "hero_kits.inc"
};

const SkillExpansionHeroKit *skillExpansion_getHeroKit(u32 heroId)
{
    return heroId < ARRAY_COUNT(HeroKits) ? HeroKits[heroId] : NULL;
}

const void *skillExpansion_getHeroTileBgAsset(u8 playerIndex)
{
    u32 heroId = PLAYER(playerIndex).heroId;
    const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(heroId);

    return SkillTreeHeroTileBgAssets[kit != NULL ? kit->artHeroId :
#if HACK_ringwraith
                                                 ringwraith_stockHero(heroId)
#else
                                     heroId < 8 ? heroId
                                                : HERO_ID_ARAGORN
#endif
    ];
}

static const SkillExpansionElitePassive *getElitePassive(u32 passiveSkillId)
{
    if (passiveSkillId >= PASSIVE_SKILL_COUNT && passiveSkillId < SKILL_EXPANSION_PASSIVE_END)
    {
        return &SkillExpansionElitePassives[passiveSkillId - PASSIVE_SKILL_COUNT];
    }
    return NULL;
}

const PassiveSkill *skillExpansion_getPassiveSkill(u32 passiveSkillId)
{
    const SkillExpansionElitePassive *elite = getElitePassive(passiveSkillId);

    if (passiveSkillId < PASSIVE_SKILL_COUNT)
    {
        return &PassiveSkills[passiveSkillId];
    }
    return elite != NULL ? &elite->skill : NULL;
}

/** Hero level required to buy the next level, -1 once maxed or for an unknown skill. */
s32 skillExpansion_getPassiveRequiredLevel(u32 passiveSkillId, u32 purchasedLevel)
{
    const SkillExpansionElitePassive *elite = getElitePassive(passiveSkillId);

    if (purchasedLevel >= SKILL_EXPANSION_REQUIRED_LEVEL_COUNT)
    {
        return -1;
    }
    if (passiveSkillId < PASSIVE_SKILL_COUNT)
    {
        return PassiveSkillRequiredLevels[passiveSkillId][purchasedLevel];
    }
    return elite != NULL ? elite->requiredLevels[purchasedLevel] : -1;
}

const void *skillExpansion_getPassiveIcon(u32 passiveSkillId)
{
    const SkillExpansionElitePassive *elite = getElitePassive(passiveSkillId);

    if (passiveSkillId < PASSIVE_SKILL_COUNT)
    {
        return PassiveSkillIconAssets[passiveSkillId];
    }
    if (elite == NULL)
    {
        return NULL;
    }
    return elite->icon != NULL ? elite->icon : PassiveSkillIconAssets[elite->iconPassiveSkillId];
}

/** Description of an elite passive, or NULL for stock skills, which use their text ID. */
const char *skillExpansion_getPassiveDescription(u32 passiveSkillId)
{
    const SkillExpansionElitePassive *elite = getElitePassive(passiveSkillId);

    return elite != NULL ? elite->description : NULL;
}

static void buildTables(u32 heroId, u8 playerIndex)
{
    SkillExpansionPlayer *sx = &skillExpansion_players[playerIndex];
    const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(heroId);
    const void *const *stockIcons = ActiveSkillIconAssets[SkillExpansionHeroSkillBlocks[heroId]];
    const ActiveSkill *stockRows = StockActiveSkills[heroId];
    const u8 *stockPassives = StockPassiveSkillIds[heroId];
    u32 i;

    if (kit != NULL)
    {
        stockIcons = kit->icons;
        stockRows = kit->activeSkills;
        stockPassives = kit->passiveSkillIds;
    }
    else if (stockRows == NULL || stockPassives == NULL)
    {
        stockRows = StockActiveSkills[HERO_ID_ARAGORN];
        stockPassives = StockPassiveSkillIds[HERO_ID_ARAGORN];
    }
    memory_memset(sx->activeSkills, 0, sizeof(sx->activeSkills));
    memory_memset(sx->activeSkillIcons, 0, sizeof(sx->activeSkillIcons));
    memory_memcpy(sx->activeSkills, stockRows, HERO_ACTIVE_SKILL_COUNT * sizeof(ActiveSkill));
    for (i = 0; i < HERO_ACTIVE_SKILL_COUNT; i++)
    {
        sx->activeSkillIcons[i] = stockIcons[i];
    }
    if (kit == NULL)
    {
        skillExpansion_wrapStockCallbacks(heroId, playerIndex);
    }

    for (i = 0; i < SKILL_EXPANSION_NEW_ACTIVE_COUNT; i++)
    {
        const SkillExpansionActiveDef *def =
            kit != NULL ? kit->newActives[i] : SkillExpansionActiveDefs[heroId][i];
        u32 row = SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + i;

        sx->activeDefs[i] = def;
        if (def == NULL)
        {
            continue;
        }
        memory_memcpy(&sx->activeSkills[row], def->row, sizeof(ActiveSkill));
        if (def->stockCallback != NULL)
        {
            sx->activeSkills[row].callback = skillExpansion_castStockCallback;
        }
        sx->activeSkillIcons[row] = def->icon;
    }

    memory_memcpy(sx->passiveSkillIds, stockPassives, HERO_PASSIVE_SKILL_COUNT);
    memory_memcpy(&sx->passiveSkillIds[HERO_PASSIVE_SKILL_COUNT],
                  kit != NULL ? kit->newPassiveSkillIds : SkillExpansionPassiveIds[heroId],
                  SKILL_EXPANSION_NEW_PASSIVE_COUNT);

    PLAYER(playerIndex).activeSkills = sx->activeSkills;
    PLAYER(playerIndex).activeSkillIcons = sx->activeSkillIcons;
    PLAYER(playerIndex).passiveSkillIds = sx->passiveSkillIds;
}

/**
 * Point the player's skill tables at the hero's expanded tables, then restore applied and current
 * levels from the purchased ones.
 *
 * REPLACES (whole function): player_initHeroLoadout.
 */
void skillExpansion_initHeroLoadout(u32 heroId, u32 playerIndex)
{
    u8 p = playerIndex;
    u32 i;

    heroId &= 0xff;
    if (p >= 2 || heroId >= SKILL_EXPANSION_HERO_COUNT)
    {
        return;
    }
    skillExpansion_runtimeBegin(p, heroId);
    if (heroId < SKILL_EXPANSION_HERO_COUNT)
    {
        buildTables(heroId, p);
    }

    for (i = 0; i < SKILL_EXPANSION_PASSIVE_SLOT_COUNT; i++)
    {
        *skillExpansion_passiveApplied(p, i) = *skillExpansion_passivePurchased(p, i);
    }
    skillExpansion_passiveApplyAllLevels(p);

#if SKILL_EXPANSION_TEST_GRANT_NEW_ACTIVES
    for (i = 0; i < SKILL_EXPANSION_NEW_ACTIVE_COUNT; i++)
    {
        u8 *purchased = skillExpansion_activePurchased(p, SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + i);

        if (skillExpansion_players[p].activeDefs[i] != NULL && *purchased == 0)
        {
            *purchased = 1;
        }
    }
#endif

    for (i = 0; i < SKILL_EXPANSION_ACTIVE_ROW_COUNT; i++)
    {
        *skillExpansion_activeLevel(p, i) = *skillExpansion_activePurchased(p, i);
    }
    skillExpansion_runtimeInstall(p);
}

/**
 * Restore both players' quick-select bindings and enable every row in the cast cycle.
 *
 * REPLACES (whole function): player_resetActiveSkillControls.
 */
void skillExpansion_resetActiveSkillControls(void)
{
    u32 p;
    u32 row;

    for (p = 0; p < 2; p++)
    {
        PLAYER(p).quickSelectActiveSkills[0] = ACTIVE_SKILL_HERBAL_HEALING;
        PLAYER(p).quickSelectActiveSkills[1] = 0;
        PLAYER(p).quickSelectActiveSkills[2] = 1;
        for (row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
        {
            *skillExpansion_activeCycleEnabled(p, row) = row != ACTIVE_SKILL_NONE;
        }
    }
}

/**
 * Reset both players' skill levels: 0, or 5 in GAME_FLAG_2 debug mode. Herbal Healing is level 1.
 *
 * REPLACES (whole function): player_resetActiveSkills.
 */
void skillExpansion_resetActiveSkills(void)
{
    u32 p;
    u32 i;
    u8 level = (g_GameFlags.p & GAME_FLAG_2) ? PASSIVE_SKILL_MAX_LEVEL : 0;

    for (p = 0; p < 2; p++)
    {
        for (i = 0; i < SKILL_EXPANSION_PASSIVE_SLOT_COUNT; i++)
        {
            *skillExpansion_passivePurchased(p, i) = level;
            *skillExpansion_passiveApplied(p, i) = level;
        }
        for (i = 0; i < SKILL_EXPANSION_ACTIVE_ROW_COUNT; i++)
        {
            u8 rowLevel = i == ACTIVE_SKILL_NONE ? 0 : level;

            *skillExpansion_activePurchased(p, i) = rowLevel;
            *skillExpansion_activeLevel(p, i) = rowLevel;
        }
        *skillExpansion_activePurchased(p, ACTIVE_SKILL_HERBAL_HEALING) = 1;
        *skillExpansion_activeLevel(p, ACTIVE_SKILL_HERBAL_HEALING) = 1;
    }
    skillExpansion_resetActiveSkillControls();
}

/**
 * Cast callback of a new row that reuses a stock callback. The stock callback reads its own row's
 * level, table row and castingActiveSkill, so this row's are swapped into def->baseRow around it.
 */
void skillExpansion_castStockCallback(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    SkillExpansionPlayer *sx = &skillExpansion_players[p];
    u8 row = PLAYER(p).castingActiveSkill;
    const SkillExpansionActiveDef *def = skillExpansion_activeDef(p, row);
    ActiveSkill savedRow;
    u8 *baseLevel;
    u8 savedLevel;

    if (def == NULL || def->stockCallback == NULL)
    {
        return;
    }

    baseLevel = skillExpansion_activeLevel(p, def->baseRow);
    savedLevel = *baseLevel;
    memory_memcpy(&savedRow, &sx->activeSkills[def->baseRow], sizeof(ActiveSkill));

    memory_memcpy(&sx->activeSkills[def->baseRow], &sx->activeSkills[row], sizeof(ActiveSkill));
    sx->activeSkills[def->baseRow].callback = def->stockCallback;
    *baseLevel = *skillExpansion_activeLevel(p, row);
    PLAYER(p).castingActiveSkill = def->baseRow;

    def->stockCallback(playerActor);

    PLAYER(p).castingActiveSkill = row;
    *baseLevel = savedLevel;
    memory_memcpy(&sx->activeSkills[def->baseRow], &savedRow, sizeof(ActiveSkill));
}

/** Cast pose for the cast state: the base row's pose, as stock rows use castingActiveSkill + 1. */
u8 skillExpansion_getCastPose(u32 playerIndex, u32 row)
{
    const SkillExpansionCastPolicy *policy = skillExpansion_getCastPolicy(playerIndex, row);

    if (policy != NULL)
    {
        return policy->pose;
    }
    return skillExpansion_baseRow(playerIndex, row) + 1;
}

const SkillExpansionCastPolicy *skillExpansion_getCastPolicy(u8 playerIndex, u32 row)
{
    const SkillExpansionActiveDef *def = skillExpansion_activeDef(playerIndex, row);
    const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(PLAYER(playerIndex).heroId);

    if (def != NULL)
    {
        return def->policy;
    }
    return kit != NULL && row < HERO_ACTIVE_SKILL_COUNT ? kit->policies[row] : NULL;
}

// Permanent legacy identities. Keep these values when moving a skill to another row.
static const u16 LegacyActiveIds[SKILL_EXPANSION_HERO_COUNT][SKILL_EXPANSION_ACTIVE_ROW_COUNT] = {
    {0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0, 0x108, 0x109, 0x10a, 0x10b, 0x10c, 0x10d},
    {0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0, 0x208, 0x209, 0x20a, 0x20b, 0x20c, 0x20d},
    {0x301, 0x302, 0x303, 0x304, 0x305, 0x306, 0, 0x308, 0x309, 0x30a, 0x30b, 0x30c, 0x30d},
    {0x401, 0x402, 0x403, 0x404, 0x405, 0x406, 0, 0x408, 0x409, 0x40a, 0x40b, 0x40c, 0x40d},
    {0x501, 0x502, 0x503, 0x504, 0x505, 0x506, 0, 0x508, 0x509, 0x50a, 0x50b, 0x50c, 0x50d},
    {0x601, 0x602, 0x603, 0x604, 0x605, 0x606, 0, 0x608, 0x609, 0x60a, 0x60b, 0x60c, 0x60d},
    {0x701, 0x702, 0x703, 0x704, 0x705, 0x706, 0, 0x708, 0x709, 0x70a, 0x70b, 0x70c, 0x70d},
    {0x801, 0x802, 0x803, 0x804, 0x805, 0x806, 0, 0x808, 0x809, 0x80a, 0x80b, 0x80c, 0x80d},
    {0x901, 0x902, 0x903, 0x904, 0x905, 0x906, 0, 0x908, 0x909, 0x90a, 0x90b, 0x90c, 0x90d},
};

u16 skillExpansion_activeId(u32 heroId, u32 row)
{
    const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(heroId);
    const SkillExpansionActiveDef *def;

    if (heroId >= SKILL_EXPANSION_HERO_COUNT || row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT ||
        row == ACTIVE_SKILL_NONE)
    {
        return 0;
    }
    if (row < HERO_ACTIVE_SKILL_COUNT)
    {
        return kit != NULL && kit->activeIds != NULL ? kit->activeIds[row]
                                                     : LegacyActiveIds[heroId][row];
    }
    def = kit != NULL
              ? kit->newActives[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW]
              : SkillExpansionActiveDefs[heroId][row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW];
    return def == NULL ? 0 : def->id != 0 ? def->id : LegacyActiveIds[heroId][row];
}

u16 skillExpansion_legacyActiveId(u32 heroId, u32 row)
{
    return heroId < ARRAY_COUNT(LegacyActiveIds) && row < SKILL_EXPANSION_ACTIVE_ROW_COUNT
               ? LegacyActiveIds[heroId][row]
               : 0;
}

u16 skillExpansion_passiveId(u32 heroId, u32 slot)
{
    const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(heroId);
    u8 id;

    if (heroId >= SKILL_EXPANSION_HERO_COUNT || slot >= SKILL_EXPANSION_PASSIVE_SLOT_COUNT)
    {
        return 0;
    }
    if (slot < HERO_PASSIVE_SKILL_COUNT)
    {
        const u8 *ids = kit != NULL ? kit->passiveSkillIds : StockPassiveSkillIds[heroId];
        id = (ids != NULL ? ids : StockPassiveSkillIds[HERO_ID_ARAGORN])[slot];
    }
    else
    {
        id = (kit != NULL ? kit->newPassiveSkillIds
                          : SkillExpansionPassiveIds[heroId])[slot - HERO_PASSIVE_SKILL_COUNT];
    }
    return id == SKILL_EXPANSION_PASSIVE_ID_NONE ? 0 : 0x8000u + id;
}
