#define SKILL_EXPANSION_HERO_STATE_WORDS 40
#define asm(...)
#include "patches/skill_expansion/skill_expansion_frodo.c"
#undef asm
#include <assert.h>
#include <stdio.h>
PlayerGlobals g_PlayerGlobals;
GameFlags g_GameFlags;
SkillExpansionPlayer skillExpansion_players[2];
u32 skillExpansion_heroState[2][SKILL_EXPANSION_HERO_STATE_WORDS];
u8 skillExpansion_heroStateOwner[2];
u8 skillExpansion_heroStateMisuse;
void skillExpansion_noteHeroStateMisuse(void) { skillExpansion_heroStateMisuse++; }
static const u16 palette[16];
const u16 *ObjPalettes[90] = {[0 ... 89] = palette};
static u8 tintStrength = 255;
void player_addCorruption(u8 p, s8 delta)
{
    s32 value = PLAYER(p).corruption + delta;
    PLAYER(p).corruption = value < 0 ? 0 : value > 100 ? 100 : value;
    if (value >= 91)
    {
        g_GameFlags.p |= GAME_FLAG_CORRUPTION_TIER_3;
    }
    else if (value >= 51)
    {
        g_GameFlags.p |= GAME_FLAG_CORRUPTION_TIER_1;
    }
}
void skillExpansion_feedback(u8 p, const char *message)
{
    (void)p;
    (void)message;
}
void hackLib_fxTintSlot(u8 slot, const u16 *base, u8 first, u8 count, u16 color, u8 strength)
{
    (void)slot;
    (void)base;
    (void)first;
    (void)count;
    (void)color;
    tintStrength = strength;
}
int main(void)
{
    Actor avatar = {0}, ring = {0};
    PLAYER(0).heroId = HERO_ID_FRODO;
    PLAYER(0).currentHp = 100;
    const u32 mask =
        GAME_FLAG_CORRUPTION_TIER_1 | GAME_FLAG_CORRUPTION_TIER_2 | GAME_FLAG_CORRUPTION_TIER_3;
    for (s32 value = 0; value <= 100; value++)
    {
        PLAYER(0).corruption = value;
        g_GameFlags.p = mask | GAME_FLAG_OVERLAY;
        addBurden(0, 0);
        assert(!!(g_GameFlags.p & GAME_FLAG_CORRUPTION_TIER_1) == (value >= 51));
        assert(!!(g_GameFlags.p & GAME_FLAG_CORRUPTION_TIER_2) == (value >= 81));
        assert(!!(g_GameFlags.p & GAME_FLAG_CORRUPTION_TIER_3) == (value >= 91));
        assert(g_GameFlags.p & GAME_FLAG_OVERLAY);
    }
    FrodoState *st = state(0);
    for (u32 row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        st->savedSpiritCosts[row] = row + 10;
    }
    PLAYER(0).corruption = 100;
    g_GameFlags.p = mask;
    st->claimFrames = 1;
    endClaim(0, st);
    assert(PLAYER(0).corruption == 60);
    assert((g_GameFlags.p & mask) == GAME_FLAG_CORRUPTION_TIER_1);
    showState(0, &avatar, st);
    assert(tintStrength == 60 * BURDEN_TINT_MAX / BURDEN_MAX);
    for (u32 row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        assert(skillExpansion_players[0].activeSkills[row].spiritCost == row + 10);
    }
    st->claimFrames = 10;
    st->tintStrength = 14;
    st->stingGlowing = TRUE;
    st->phialFrames = st->calmFrames = st->knifeFrames = 50;
    st->heldRing = PLAYER(0).ringActor = &ring;
    ring.actionState = RING_STATE_WORN;
    ring.actionStateTimer = 60;
    PLAYER(0).currentHp = 0;
    PLAYER(0).corruption = 100;
    g_GameFlags.p = mask;
    skillExpansion_frodoTick(0, &avatar);
    assert(!PLAYER(0).corruption && !(g_GameFlags.p & mask));
    assert(!st->claimFrames && !st->phialFrames && !st->stingGlowing);
    assert(!st->calmFrames && !st->knifeFrames && !st->heldRing);
    assert(!st->tintStrength && !tintStrength && ring.actionStateTimer == 1);
    skillExpansion_frodoTick(0, &avatar);
    PLAYER(0).currentHp = 100;
    PLAYER(0).corruption = 60;
    skillExpansion_frodoCleanup(0, NULL);
    assert(PLAYER(0).corruption == 60);
    puts("PASS: Frodo tier transitions, Claim expiry, death cleanup and palette restoration");
}

#include <stdlib.h>
// Unexpected native calls fail this lifecycle-only fixture.
void *memory_memset(void *dest, u32 value, size_t count)
{
    abort();
}
s32 skillExpansion_getPassiveLevel(u8 playerIndex, u8 passiveSkillId)
{
    abort();
}
void skillExpansion_spawnBurstFx(Actor *target, u8 variant, s32 sfx)
{
    abort();
}
Actor *hackLib_fxBurst(Actor *on, u8 shape, HackLibFxColor color, u8 layer)
{
    abort();
}
Actor *hackLib_fxAt(Actor *origin, const Vector2Fp16 *pos, u8 shape, HackLibFxColor color)
{
    abort();
}
bool32 skillExpansion_isFoe(Actor *actor)
{
    abort();
}
HackLibDamageResult skillExpansion_dealDamage(Actor *attacker, Actor *victim, u16 damage, u8 kind,
                                              u8 abilityRow)
{
    abort();
}
u32 sfx_play(s32 sfxId)
{
    abort();
}
Actor *hackLib_nextTarget(const HackLibTargetQuery *query, Actor *cursor)
{
    abort();
}
Actor *skill_active_frodoKnifeToss(Actor *playerActor)
{
    abort();
}
u32 skill_active_frodoGaladrielsCloak(Actor *playerActor)
{
    abort();
}
void skill_active_frodoRingsPersuasion(Actor *playerActor)
{
    abort();
}
Actor *actor_iterateInRadius(Actor *center, Actor *cursor, s32 radius)
{
    abort();
}
void actor_applyPersuasion(Actor *enemy, u16 frames)
{
    abort();
}
void player_addHp(u8 playerIndex, s16 delta, bool flinchOnDamage)
{
    abort();
}
void player_addSpirit(u8 playerIndex, s16 delta)
{
    abort();
}
s32 rng_rollRange(s32 lo, s32 hi)
{
    abort();
}
bool actor_isStunnable(Actor *actor)
{
    abort();
}
Actor *fx_attachStatusEffect(Actor *parent, Actor *target, u16 duration, u8 kind, u8 arg4, u8 arg5,
                             u8 flagBit)
{
    abort();
}
Actor *hackLib_fxTimed(Actor *on, u8 shape, HackLibFxColor color, s32 frames, u8 layer)
{
    abort();
}
bool32 hackLib_fxHold(Actor *fx, u16 frames)
{
    abort();
}
u32 skillExpansion_burstDamage(const SkillExpansionBurst *burst)
{
    abort();
}
Actor *hackLib_fxPace(Actor *fx, u8 frameTicks)
{
    abort();
}
bool hackLib_findLanding(const Actor *actor, const Vector2Fp16 *preferred, Vector2Fp16 *result)
{
    abort();
}
Actor *skill_active_frodoTheOneRing(Actor *playerActor)
{
    abort();
}
void hackLib_drawHudMeter(const HackLibHudMeter *meter, s32 value, s32 max,
                          const HackLibHudMeterLook *look, const u8 *marks, u32 markCount)
{
    abort();
}
const EnemyTypeInfo EnemyTypeInfos[83] = {0};
const HackLibHudMeter HackLibHudGapMeter = {0};
PlayerHud g_PlayerHuds[2];
