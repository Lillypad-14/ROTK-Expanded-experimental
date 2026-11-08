// Versus: player-vs-player for co-op. See versus.h.
#include "patches/versus/versus.h"
#include "combat.h"
#include "font.h"
#include "game.h"
#include "gfx.h"
#include "multiplayer.h"
#include "input.h"
#include "lib/mission_scripts.h"
#include "lib/pc_host.h"
#include "memory.h"
#include "patches/mod_save/save_sram.h"
#include "lib/actor_ref.h"
#include "lib/combat_source.h"
#include "lib/summon.h"
#include "math.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "player.h"
#include "save.h"
#include "scene.h"
#include "scene/gameplay.h"
#include "sfx.h"
#include "stats.h"
#include "variables.h"

// Timers count player ticks, and co-op ticks the world every other frame: 30 a second.
// A knocked-out player stays down 1.5 s before respawning, then is protected for 2 s.
#define VERSUS_DOWN_TICKS 45
#define VERSUS_SHIELD_TICKS 60
// The result shows 6 s before both players go back to the co-op level select.
#define VERSUS_OVER_TICKS 180
// No single skill hit takes more than this share of a player's max HP, so no skill one-shots.
#define VERSUS_SKILL_HIT_MAX_PERCENT 40
// Parent links followed from a projectile, summon or effect to the player behind it.
#define VERSUS_OWNER_DEPTH 4
// A stunned player stays stunned at most 1.5 s, then can't be stunned again for 1.5 s.
#define VERSUS_STUN_MAX_TICKS 45
#define VERSUS_STUN_IMMUNE_TICKS 45

static const u8 KoTargets[VERSUS_KO_TARGET_COUNT] = {3, 5, 7, 10};
static const u16 DamagePercents[VERSUS_DAMAGE_COUNT] = {100, 150, 200, 300};

enum VersusPhase
{
    VERSUS_PHASE_OFF, // not a duel
    VERSUS_PHASE_FIGHT,
    VERSUS_PHASE_OVER,
};

// The modcode RAM budget is nearly spent, so the match state is packed (spawn points in whole
// pixels) and the messages are written into the HUD's own text buffers.
typedef struct VersusDuel
{
    u8 phase : 2;
    u8 announced : 1;
    u8 started : 2; // bit per player: its spawn point is captured
    u8 pvpHits : 2; // nesting depth of combat_resolvePvpAttack (a riposte re-enters it once)
    u8 pvpAttacker : 1; // the player whose hit combat_resolvePvpAttack is resolving
    u8 score[2];
    u8 downFrames[2];
    u8 shieldFrames[2];
    u8 acting; // VERSUS_NO_PLAYER or the player whose tick is running (fills the u16's padding)
    u8 stunTicks[2];  // ticks left of a player's current stun, 0 when not stunned
    u8 stunImmune[2]; // ticks left in which a new stun on the player is shaken off at once
    u16 overFrames;
    u16 spawnX[2];
    u16 spawnY[2];
} VersusDuel;

struct VersusState g_VersusState;
static VersusDuel Duel;
// Multiplayer ID of the console whose global mod options a co-op session plays with.
#define SESSION_HOST 0

// This patch turned the mission's spawns and scripts off (outside Duel: a match resets it).
static u8 SuppressedMission;

void versus_loadState(void)
{
    GLOBAL_MOD_OPTIONS_READ(versusState, &g_VersusState);
    if (g_VersusState.friendlyFire >= VERSUS_FRIENDLY_FIRE_COUNT)
    {
        g_VersusState = (struct VersusState){.friendlyFire = VERSUS_FRIENDLY_FIRE_STOCK,
                                             .duel = FALSE,
                                             .duelKoTarget = 1,
                                             .duelEnemies = FALSE,
                                             .duelDamage = 2};
    }
}

static bool32 coopActive(void)
{
    return (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE) != 0;
}

static bool32 duelRunning(void)
{
    return Duel.phase != VERSUS_PHASE_OFF && coopActive() && g_SceneCurrent.id == SCENE_ID_GAMEPLAY;
}

bool32 versus_friendlyFireHits(void)
{
    if (duelRunning())
    {
        return Duel.phase == VERSUS_PHASE_FIGHT;
    }
    switch (g_VersusState.friendlyFire)
    {
    case VERSUS_FRIENDLY_FIRE_ALWAYS:
        return TRUE;
    case VERSUS_FRIENDLY_FIRE_NEVER:
        return FALSE;
    default:
        return (g_SaveGlobals.d.optionsFlags.p & OPTION_FLAG_DIFFICULTY_GRUELING) != 0;
    }
}

// The stock body past the 8-byte prologue hookReplace overwrote.
asm(".section .text\n"
    ".balign 4\n.thumb\n.global versus_originalPvpAttack\n.thumb_func\n"
    "versus_originalPvpAttack:\n"
    "push {r4-r7,lr}\nmov r7, r10\nmov r6, r9\nmov r5, r8\n"
    "ldr r3, =combat_resolvePvpAttack + 8\nbx r3\n.pool\n");

bool versus_originalPvpAttack(Actor *attacker, Actor *victim);

bool versus_resolvePvpAttack(Actor *attacker, Actor *victim)
{
    bool hit;
    u8 target = victim->as.combat.playerIndex & 1;
    s32 hpBefore = PLAYER(target).currentHp;

    u8 previousAttacker = Duel.pvpAttacker;

    Duel.pvpAttacker = attacker->as.combat.playerIndex & 1;
    Duel.pvpHits++;
    hit = versus_originalPvpAttack(attacker, victim);
    if (hit)
    {
        hackLib_probePlayer(HACKLIB_PROBE_HIT, victim);
        skillExpansion_playerHitPlayer(attacker, victim,
                                       hackLib_damageResult(hpBefore, PLAYER(target).currentHp, TRUE), TRUE);
    }
    Duel.pvpHits--;
    Duel.pvpAttacker = previousAttacker;
    return hit;
}

static void publishScore(void);

void versus_gameplayEnter(void)
{
    u32 mode = SCENE_ARG_GAMEPLAY_MODE;

    // The overlay menus (pause, backpack, skill tree) park the world and resume it: same match.
    if (mode == GAMEPLAY_MODE_RESUME || mode == GAMEPLAY_MODE_3)
    {
        return;
    }
    memory_memset(&Duel, 0, sizeof(Duel));
    Duel.acting = VERSUS_NO_PLAYER;
    Duel.phase = coopActive() && g_VersusState.duel ? VERSUS_PHASE_FIGHT : VERSUS_PHASE_OFF;
    // Spawn groups and story scripts run during the load, so the switch is set here. Only a
    // switch this patch set is cleared, so the Gauntlet's own use of it is left alone.
    if (SuppressedMission)
    {
        hackLib_suppressMissionScripts = FALSE;
        SuppressedMission = FALSE;
    }
    if (Duel.phase == VERSUS_PHASE_FIGHT && !g_VersusState.duelEnemies)
    {
        hackLib_suppressMissionScripts = TRUE;
        SuppressedMission = TRUE;
    }
    publishScore();
}

/** The scoreboard for the PC host's overlay (absent on a GBA). */
static void publishScore(void)
{
    volatile PcHostInfo *info = hackLib_pcHostInfoWritable();

    if (info != NULL && info->version >= 3)
    {
        info->duel = Duel.phase == VERSUS_PHASE_OFF
                         ? 0
                         : PC_HOST_DUEL(Duel.phase, Duel.score[0], Duel.score[1],
                                        KoTargets[g_VersusState.duelKoTarget]);
    }
}

static char *appendText(char *out, const char *text)
{
    while (*text != '\0')
    {
        *out++ = *text++;
    }
    return out;
}

static char *appendNumber(char *out, u32 value)
{
    if (value >= 10)
    {
        out = appendNumber(out, value / 10);
    }
    *out++ = (char)('0' + value % 10);
    return out;
}

static char *appendPlayer(char *out, u8 playerIndex)
{
    out = appendText(out, "Player ");
    return appendNumber(out, playerIndex + 1);
}

static char *appendScore(char *out, u8 playerIndex)
{
    out = appendNumber(out, Duel.score[playerIndex]);
    *out++ = '-';
    return appendNumber(out, Duel.score[playerIndex ^ 1]);
}

/** Player 0's HUD text buffer, where a message is written before announce() copies it. */
static char *nextMessage(void)
{
    return g_PlayerHuds[0].textBuffer;
}

/** Show a message on both players' HUDs; the feedback line redraws a HUD text buffer each time. */
static void announce(const char *message)
{
    u32 i = 0;

    do
    {
        g_PlayerHuds[1].textBuffer[i] = message[i];
    } while (message[i++] != '\0');
    skillExpansion_feedback(0, g_PlayerHuds[0].textBuffer);
    skillExpansion_feedback(1, g_PlayerHuds[1].textBuffer);
}

static void knockOut(u8 playerIndex)
{
    Actor *actor = PLAYER(playerIndex).ownerActor;
    u8 winner = playerIndex ^ 1;
    char *message = nextMessage();
    char *out = message;

    PLAYER(playerIndex).currentHp = 1;
    g_PlayerHuds[playerIndex].flags.p |= PLAYERHUD_FLAG_HP_DIRTY;
    Duel.downFrames[playerIndex] = VERSUS_DOWN_TICKS;
    Duel.shieldFrames[playerIndex] = 0;
    if (actor != NULL)
    {
        actor->flags.p |= ACTOR_FLAG_INVULNERABLE;
        actor_resetActionState(actor);
        actor->actionState = ACTOR_STATE_KNOCKED_DOWN;
        actor->field_0x64[0] |= 2;
    }
    if (Duel.score[winner] < 99)
    {
        Duel.score[winner]++;
    }
    out = appendPlayer(out, winner);
    if (Duel.score[winner] >= KoTargets[g_VersusState.duelKoTarget])
    {
        Duel.phase = VERSUS_PHASE_OVER;
        Duel.overFrames = VERSUS_OVER_TICKS;
        out = appendText(out, " wins the duel! ");
        sfx_play(SFX_LEVEL_UP);
    }
    else
    {
        out = appendText(out, " scores! ");
        sfx_play(SFX_COMBAT_KNOCKDOWN);
    }
    out = appendScore(out, winner);
    *out = '\0';
    announce(message);
}

static void respawn(u8 playerIndex)
{
    Actor *actor = PLAYER(playerIndex).ownerActor;

    PLAYER(playerIndex).currentHp = PLAYER_STAT(playerIndex, STAT_MAX_HP);
    PLAYER(playerIndex).currentSpirit = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
    g_PlayerHuds[playerIndex].flags.p |= PLAYERHUD_FLAG_HP_DIRTY | PLAYERHUD_FLAG_SPIRIT_DIRTY;
    if (actor != NULL)
    {
        actor_resetActionState(actor);
        actor->coords.x = (s32)Duel.spawnX[playerIndex] << FP16_16_SHIFT;
        actor->coords.y = (s32)Duel.spawnY[playerIndex] << FP16_16_SHIFT;
        actor->velocity.x = 0;
        actor->velocity.y = 0;
    }
    Duel.shieldFrames[playerIndex] = VERSUS_SHIELD_TICKS;
}

static void endStun(Actor *actor)
{
    actor->actionState = ACTOR_STATE_IDLE;
    actor->actionStateTimer = 0;
    actor->field_0x64[0] &= ~2;
}

/**
 * A skill's stun on a player (ACTOR_STATE_STUNNED, which the stock game only gives enemies, whose
 * tick ends it). The player tick never ends it, so it is timed here, capped, and followed by a short
 * immunity.
 */
static void tickPlayerStun(u8 playerIndex, Actor *actor)
{
    if (actor->actionState != ACTOR_STATE_STUNNED)
    {
        Duel.stunTicks[playerIndex] = 0;
        if (Duel.stunImmune[playerIndex] != 0)
        {
            Duel.stunImmune[playerIndex]--;
        }
        return;
    }
    if (Duel.stunTicks[playerIndex] == 0)
    {
        if (Duel.stunImmune[playerIndex] != 0)
        {
            endStun(actor);
            return;
        }
        Duel.stunTicks[playerIndex] =
            actor->actionStateTimer <= 0
                ? 1
                : (actor->actionStateTimer < VERSUS_STUN_MAX_TICKS ? actor->actionStateTimer
                                                                   : VERSUS_STUN_MAX_TICKS);
    }
    PLAYER_KEYS_CURRENT(playerIndex) = 0;
    PLAYER_KEYS_PRESSED(playerIndex) = 0;
    PLAYER_KEYS_RELEASED(playerIndex) = 0;
    if (--Duel.stunTicks[playerIndex] == 0)
    {
        endStun(actor);
        Duel.stunImmune[playerIndex] = VERSUS_STUN_IMMUNE_TICKS;
    }
}

void versus_playerTick(u8 playerIndex)
{
    Actor *actor = PLAYER(playerIndex).ownerActor;

    if (playerIndex >= 2 || actor == NULL)
    {
        return;
    }
    tickPlayerStun(playerIndex, actor);
    if (!duelRunning())
    {
        return;
    }
    if (playerIndex == 0)
    {
        publishScore();
    }
    if (!(Duel.started & (1 << playerIndex)))
    {
        Duel.started |= 1 << playerIndex;
        Duel.spawnX[playerIndex] = (u16)(actor->coords.x >> FP16_16_SHIFT);
        Duel.spawnY[playerIndex] = (u16)(actor->coords.y >> FP16_16_SHIFT);
    }
    if (!Duel.announced && Duel.started == 3)
    {
        char *message = nextMessage();
        char *out = appendText(message, "Duel! First to ");

        out = appendNumber(out, KoTargets[g_VersusState.duelKoTarget]);
        out = appendText(out, " KOs wins.");
        *out = '\0';
        Duel.announced = TRUE;
        announce(message);
    }
    if (Duel.phase == VERSUS_PHASE_OVER)
    {
        actor->flags.p |= ACTOR_FLAG_INVULNERABLE;
        if (playerIndex == 0 && Duel.overFrames != 0 && --Duel.overFrames == 0)
        {
            Duel.phase = VERSUS_PHASE_OFF;
            publishScore();
            game_requestSceneChange(SCENE_ID_MULTIPLAYER_LEVEL_SELECT);
        }
        return;
    }
    if (PLAYER(playerIndex).currentHp <= 0 && Duel.downFrames[playerIndex] == 0)
    {
        knockOut(playerIndex); // HP set directly, past player_addHp
        return;
    }
    if (Duel.downFrames[playerIndex] != 0)
    {
        // Down: no moves, attacks or casts (the stock tick reads these keys after this).
        PLAYER_KEYS_CURRENT(playerIndex) = 0;
        PLAYER_KEYS_PRESSED(playerIndex) = 0;
        PLAYER_KEYS_RELEASED(playerIndex) = 0;
        actor->flags.p |= ACTOR_FLAG_INVULNERABLE;
        if (--Duel.downFrames[playerIndex] == 0)
        {
            respawn(playerIndex);
        }
        return;
    }
    if (Duel.shieldFrames[playerIndex] != 0)
    {
        if (--Duel.shieldFrames[playerIndex] == 0)
        {
            actor->flags.p &= ~ACTOR_FLAG_INVULNERABLE;
        }
        else
        {
            actor->flags.p |= ACTOR_FLAG_INVULNERABLE;
        }
    }
}

s32 versus_adjustHpChange(u8 playerIndex, s32 amount)
{
    if (amount >= 0 || playerIndex >= 2 || !duelRunning())
    {
        return amount;
    }
    if (Duel.phase == VERSUS_PHASE_OVER || Duel.downFrames[playerIndex] != 0 ||
        Duel.shieldFrames[playerIndex] != 0)
    {
        return 0;
    }
    if (Duel.pvpHits != 0)
    {
        // A hit already swinging when its attacker went down doesn't count.
        if (Duel.downFrames[Duel.pvpAttacker] != 0)
        {
            return 0;
        }
        amount = amount * DamagePercents[g_VersusState.duelDamage] / 100;
    }
    return amount < -32767 ? -32767 : amount;
}

void versus_afterHpChange(u8 playerIndex)
{
    if (playerIndex < 2 && duelRunning() && Duel.phase == VERSUS_PHASE_FIGHT &&
        PLAYER(playerIndex).currentHp <= 0 && Duel.downFrames[playerIndex] == 0)
    {
        knockOut(playerIndex);
    }
}

u8 versus_beginActing(u8 playerIndex)
{
    u8 previous = Duel.acting;

    Duel.acting = playerIndex < 2 ? playerIndex : VERSUS_NO_PLAYER;
    return previous;
}

void versus_endActing(u8 previous)
{
    Duel.acting = previous;
}

u8 versus_playerOf(Actor *actor)
{
    u32 depth;

    for (depth = 0; actor != NULL && depth < VERSUS_OWNER_DEPTH; depth++)
    {
        if (skillExpansion_isPlayerAvatar(actor))
        {
            return actor->as.combat.playerIndex;
        }
        actor = actor->parentActor;
    }
    return VERSUS_NO_PLAYER;
}

/** The player behind the damage being resolved, else the player whose tick is running. */
static u8 attackingPlayer(void)
{
    const HackLibDamageSource *source = hackLib_currentDamageSource();
    u8 player = source != NULL ? versus_playerOf(source->origin) : VERSUS_NO_PLAYER;

    return player < 2 ? player : Duel.acting;
}

static bool32 isFoeOf(u8 attacker, Actor *actor)
{
    u8 target;

    if (actor == NULL || attacker >= 2 || !duelRunning() || Duel.phase != VERSUS_PHASE_FIGHT)
    {
        return FALSE;
    }
    target = actor->as.combat.playerIndex;
    return target < 2 && target != attacker && actor == PLAYER(target).ownerActor &&
           PLAYER(target).currentHp > 0 && Duel.downFrames[target] == 0 &&
           Duel.shieldFrames[target] == 0 && !(actor->flags.p & ACTOR_FLAG_INVULNERABLE);
}

bool32 versus_isDuelFoe(Actor *actor)
{
    return isFoeOf(attackingPlayer(), actor);
}

bool32 versus_isFoeOfSource(Actor *source, Actor *actor)
{
    u8 attacker = versus_playerOf(source);

    return isFoeOf(attacker < 2 ? attacker : attackingPlayer(), actor);
}

HackLibDamageResult versus_skillHitsPlayer(Actor *source, Actor *victim, u16 damage)
{
    u8 target = victim->as.combat.playerIndex;
    u8 attacker = versus_playerOf(source);
    s32 hpBefore;
    s32 cap;
    u8 previousAttacker = Duel.pvpAttacker;

    if (attacker >= 2)
    {
        attacker = attackingPlayer();
    }
    if (!isFoeOf(attacker, victim) || damage == 0)
    {
        return (HackLibDamageResult){0};
    }
    hpBefore = PLAYER(target).currentHp;
    cap = PLAYER_STAT(target, STAT_MAX_HP) * VERSUS_SKILL_HIT_MAX_PERCENT / 100;
    // Marked as a hit between players, so the duel scales it like a melee hit.
    Duel.pvpAttacker = attacker;
    Duel.pvpHits++;
    player_subtractHp(target, damage < cap ? damage : (cap > 0 ? cap : 1), TRUE);
    hackLib_probePlayer(HACKLIB_PROBE_HIT, victim);
    Duel.pvpHits--;
    skillExpansion_playerHitPlayer(PLAYER(attacker).ownerActor, victim,
                                   hackLib_damageResult(hpBefore, PLAYER(target).currentHp, TRUE), FALSE);
    Duel.pvpAttacker = previousAttacker;
    return hackLib_damageResult(hpBefore, PLAYER(target).currentHp, TRUE);
}

// The label on the stock co-op level select: its text layer (BG2, the font target the scene set),
// centred under the difficulty rows (glyph tiles 0x65..0x92, y 0x68..0x78) on their x.
#define VERSUS_LABEL_BG 2
#define VERSUS_LABEL_TILE 0xa0
#define VERSUS_LABEL_X 0xc8
#define VERSUS_LABEL_Y 0x88

void versus_drawCoopMenuLabel(void)
{
    char *message = nextMessage();
    char *out = message;

    if (g_SceneCurrent.id != SCENE_ID_MULTIPLAYER_LEVEL_SELECT)
    {
        return;
    }
    if (g_VersusState.duel)
    {
        out = appendText(out, "Duel: ");
        out = appendNumber(out, KoTargets[g_VersusState.duelKoTarget]);
        out = appendText(out, " KOs");
    }
    else
    {
        out = appendText(out, "START: Duel");
    }
    *out = ' ';
    gfx_fillBgRect(VERSUS_LABEL_BG, (VERSUS_LABEL_X - 40) / 8, VERSUS_LABEL_Y / 8, 10, 2, 0);
    font_setStyle(FONT_STYLE_8PX_A, g_VersusState.duel ? 6 : 0xc, 0);
    font_drawString(VERSUS_LABEL_TILE, VERSUS_LABEL_X, VERSUS_LABEL_Y, message, 1);
}

bool32 versus_coopMenuTick(void)
{
    if (!coopActive() || !(PLAYER_KEYS_PRESSED(0) & START_BUTTON))
    {
        return FALSE;
    }
    g_VersusState.duel ^= 1;
    // Each console plays the host's options for the session; only the host keeps the change.
    if (multiplayer_getMultiPlayerId() == SESSION_HOST)
    {
        GLOBAL_MOD_OPTIONS_WRITE(versusState, &g_VersusState);
    }
    sfx_play(SFX_MENU_CONFIRM);
    versus_drawCoopMenuLabel();
    return TRUE;
}

#if HACK_mod_options
static const char *const OffOnNames[2] = {"Off", "On"};
static const char *const FriendlyFireNames[VERSUS_FRIENDLY_FIRE_COUNT] = {"Grueling", "Always", "Never"};
static const char *const KoTargetNames[VERSUS_KO_TARGET_COUNT] = {"3", "5", "7", "10"};
static const char *const DamageNames[VERSUS_DAMAGE_COUNT] = {"100%", "150%", "200%", "300%"};

static void writeState(void)
{
    GLOBAL_MOD_OPTIONS_WRITE(versusState, &g_VersusState);
}

#define VERSUS_SETTING(field)                                                                      \
    static u8 get_##field(void)                                                                    \
    {                                                                                              \
        return g_VersusState.field;                                                                \
    }                                                                                              \
    static void set_##field(u8 value)                                                              \
    {                                                                                              \
        g_VersusState.field = value;                                                               \
        writeState();                                                                              \
    }

VERSUS_SETTING(duel)
VERSUS_SETTING(duelKoTarget)
VERSUS_SETTING(duelEnemies)
VERSUS_SETTING(duelDamage)
VERSUS_SETTING(friendlyFire)

const struct ModOptionEntry VersusOptions[VERSUS_OPTION_COUNT] = {
    {"Duel", OffOnNames, 2, 0, get_duel, set_duel, "Co-op missions become a\nduel: score KOs to win."},
    {"KOs to Win", KoTargetNames, VERSUS_KO_TARGET_COUNT, 0, get_duelKoTarget, set_duelKoTarget,
     "The first player to this\nmany KOs wins the duel."},
    {"Duel Enemies", OffOnNames, 2, 0, get_duelEnemies, set_duelEnemies,
     "Keep the mission's enemies\nin a duel."},
    {"Duel Damage", DamageNames, VERSUS_DAMAGE_COUNT, 0, get_duelDamage, set_duelDamage,
     "Scales hits between the\nplayers in a duel."},
    {"Friendly Fire", FriendlyFireNames, VERSUS_FRIENDLY_FIRE_COUNT, 0, get_friendlyFire,
     set_friendlyFire, "Co-op: when your hits land\non your partner."},
};
#endif

// The actors a player's stock area skills can hit (actor_iterateInRadius's flag test).
#define VERSUS_ITERATE_FLAGS_MASK (ACTOR_FLAG_10 | ACTOR_FLAG_18 | ACTOR_FLAG_28)

/**
 * actor_iterateInRadius, which also yields the duel opponent of the player behind the damage being
 * resolved (else behind center), so stock area skills reach the other player.
 *
 * REPLACES (whole function): actor_iterateInRadius.
 */
Actor *versus_iterateInRadius(Actor *center, Actor *cursor, s32 radius)
{
    u8 attacker = VERSUS_NO_PLAYER;
    Actor *actor;

    if (duelRunning() && Duel.phase == VERSUS_PHASE_FIGHT)
    {
        attacker = attackingPlayer();
        if (attacker >= 2)
        {
            attacker = versus_playerOf(center);
        }
    }
    for (actor = cursor == NULL ? g_ActorListMain.head : cursor->next; actor != NULL;
         actor = actor->next)
    {
        if ((actor->flags.p & ACTOR_FLAG_ON_SCREEN) && actor->actionState != ACTOR_STATE_UNKNOWN_11 &&
            actor->actionState != ACTOR_STATE_UNKNOWN_15 &&
            actor->actionState != ACTOR_STATE_UNKNOWN_04 &&
            math_vector2_distance(*(Vector2Int *)&center->renderCoords,
                                  *(Vector2Int *)&actor->renderCoords) <= (u32)radius &&
            ((actor->flags.p & VERSUS_ITERATE_FLAGS_MASK) || isFoeOf(attacker, actor)))
        {
            return actor;
        }
    }
    return NULL;
}

// The enemy walk's view around the player's camera, in pixels.
#define VERSUS_VIEW_MARGIN 40
#define VERSUS_VIEW_WIDTH 240
#define VERSUS_VIEW_HEIGHT 160

/**
 * The stock enemy walk (0x080013a8): the next actor after cursor, on the main list, in view of
 * playerActor's camera (or always in view), that a player's attack can hit. In a duel it also yields
 * the player's opponent, so stock skills and their cast checks reach the other player.
 *
 * REPLACES (whole function): the stock enemy walk at 0x080013a8 (findNextEnemyStock).
 */
Actor *versus_findNextEnemy(Actor *playerActor, Actor *cursor)
{
    u8 attacker = duelRunning() && Duel.phase == VERSUS_PHASE_FIGHT ? versus_playerOf(playerActor)
                                                                    : VERSUS_NO_PLAYER;
    Actor *actor;

    for (actor = cursor == NULL ? g_ActorListMain.head : cursor->next; actor != NULL;
         actor = actor->next)
    {
        bool32 inView = actor->field_0xfe == 1;

        if (!inView)
        {
            Vector2Int camera;
            Vector2Int position;

            gfx_getCameraPosition(&camera, playerActor->as.combat.playerIndex);
            math_vector2_fromFp16(&actor->coords, &position);
            inView = position.x >= camera.x - VERSUS_VIEW_MARGIN && position.y >= camera.y - VERSUS_VIEW_MARGIN &&
                     position.x <= camera.x + VERSUS_VIEW_WIDTH + VERSUS_VIEW_MARGIN &&
                     position.y <= camera.y + VERSUS_VIEW_HEIGHT + VERSUS_VIEW_MARGIN;
        }
        if (inView && ((actor->flags.p & VERSUS_ITERATE_FLAGS_MASK) || isFoeOf(attacker, actor)) &&
            actor->actionState != ACTOR_STATE_UNKNOWN_11 && actor->actionState != ACTOR_STATE_UNKNOWN_15 &&
            actor->actionState != ACTOR_STATE_UNKNOWN_04)
        {
            return actor;
        }
    }
    return NULL;
}

u8 hackLib_beginActingFor(Actor *actor)
{
    return versus_beginActing(versus_playerOf(actor));
}

void hackLib_endActing(u8 previous)
{
    versus_endActing(previous);
}

/**
 * combat_computePvpDamage with the attacking hero's melee bonuses (passives, kits, twists), which
 * the stock PvP blow leaves out. Called from combat_resolvePvpAttack's damage call.
 */
s32 versus_computePvpDamage(Actor *attacker, Actor *victim, bool crit)
{
    return skillExpansion_scalePvpMelee(attacker, victim, combat_computePvpDamage(attacker, victim, crit));
}
