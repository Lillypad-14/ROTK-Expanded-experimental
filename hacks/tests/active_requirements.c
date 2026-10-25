#include "patches/skill_expansion/skill_expansion_active.c"
#include <assert.h>
#include <stdio.h>

PlayerGlobals g_PlayerGlobals;
GameFlags g_GameFlags;
PlayerHud g_PlayerHuds[2];
u32 g_CurrentMissionId;
u8 g_MissionVariant;
u16 g_PlayerKeysCurrent[2], g_PlayerKeysPressed[2];
SkillExpansionPlayer skillExpansion_players[2];
SkillExpansionRuntime skillExpansion_runtime[2];
void skillExpansion_passiveAddTemporaryLevels(u8 playerIndex, u8 count)
{
    (void)playerIndex;
    (void)count;
}
void skillExpansion_passiveRemoveTemporaryLevels(u8 playerIndex, u8 count)
{
    (void)playerIndex;
    (void)count;
}
bool skillExpansion_isTimelineBusy(u8 playerIndex)
{
    (void)playerIndex;
    return FALSE;
}
const SkillExpansionHeroKit *skillExpansion_getHeroKit(u32 heroId)
{
    (void)heroId;
    return NULL;
}
const SkillExpansionCastPolicy *skillExpansion_getCastPolicy(u8 playerIndex, u32 row)
{
    (void)playerIndex;
    (void)row;
    return NULL;
}
void skill_active_cast(u8 playerIndex, u8 row)
{
    (void)playerIndex;
    (void)row;
}
void skillExpansion_rejectCast(u8 playerIndex, u8 row)
{
    (void)playerIndex;
    (void)row;
}
static const ItemMetadata UniqueMetadata[] = {
    {.flags.p = ITEM_FAMILY_SWORD},
    {.flags.p = ITEM_FAMILY_STAFF},
    {.flags.p = ITEM_FAMILY_SHIELD},
    {.flags.p = ITEM_FAMILY_AXE},
};
const ItemTypeInfo ItemTypeInfos[15] = {
    [ITEM_TYPE_UNIQUE] = {.baseMetadata = UniqueMetadata},
};
s16 skill_active_getLeveledValue(u8 playerIndex, u8 activeSkillIndex, u8 valueIndex, u8 levelCount)
{
    (void)playerIndex;
    (void)activeSkillIndex;
    (void)valueIndex;
    return levelCount;
}

int main(void)
{
    Actor actor = {0};
    PLAYER(0).heroId = HERO_ID_GANDALF;
    PLAYER(0).inventory.slots.mainhand = EMPTY_ITEM;
    PLAYER(0).inventory.slots.offhand = EMPTY_ITEM;
    assert(!meetsHeroRequirements(0, &actor, ACTIVE_SKILL_GANDALF_SWORD_OF_POWER));
    assert(!meetsHeroRequirements(0, &actor, ACTIVE_SKILL_GANDALF_LIGHTSTRIKE));
    Item item = EMPTY_ITEM;
    item.d.itemType = ITEM_TYPE_UNIQUE;
    for (u32 base = 0; base < ARRAY_COUNT(UniqueMetadata); base++)
    {
        item.d.baseIndex = base;
        PLAYER(0).inventory.slots.mainhand = item;
        PLAYER(0).inventory.slots.offhand = item;
        assert(!!meetsHeroRequirements(0, &actor, ACTIVE_SKILL_GANDALF_SWORD_OF_POWER) ==
               (base == 0));
        assert(!!meetsHeroRequirements(0, &actor, ACTIVE_SKILL_GANDALF_LIGHTSTRIKE) == (base == 1));
        PLAYER(0).heroId = HERO_ID_EOWYN;
        assert(!!meetsHeroRequirements(0, &actor, ACTIVE_SKILL_EOWYN_SHIELD_BASH) == (base == 2));
        PLAYER(0).heroId = HERO_ID_GANDALF;
    }
    item.d.itemType = ITEM_TYPE_SWORD_1H;
    PLAYER(0).inventory.slots.mainhand = item;
    item.d.itemType = ITEM_TYPE_STAFF;
    PLAYER(0).inventory.slots.offhand = item;
    assert(meetsHeroRequirements(0, &actor, ACTIVE_SKILL_GANDALF_SWORD_OF_POWER));
    assert(meetsHeroRequirements(0, &actor, ACTIVE_SKILL_GANDALF_LIGHTSTRIKE));
    PLAYER(0).heroId = HERO_ID_EOWYN;
    item.d.itemType = ITEM_TYPE_SHIELD;
    PLAYER(0).inventory.slots.offhand = item;
    assert(meetsHeroRequirements(0, &actor, ACTIVE_SKILL_EOWYN_SHIELD_BASH));
    PLAYER(0).inventory.slots.offhand = EMPTY_ITEM;
    assert(!meetsHeroRequirements(0, &actor, ACTIVE_SKILL_EOWYN_SHIELD_BASH));
    puts("PASS: native and unique weapon families satisfy the correct skill requirements");
    return 0;
}
