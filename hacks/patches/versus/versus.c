// Versus: player-vs-player for co-op. See versus.h.
#include "patches/versus/versus.h"
#include "font.h"
#include "game.h"
#include "gfx.h"
#include "multiplayer.h"
#include "input.h"
#include "lib/mission_scripts.h"
#include "lib/pc_host.h"
#include "memory.h"
#include "patches/mod_save/save_sram.h"
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

    u8 previousAttacker = Duel.pvpAttacker;

    Duel.pvpAttacker = attacker->as.combat.playerIndex & 1;
    Duel.pvpHits++;
    hit = versus_originalPvpAttack(attacker, victim);
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

void versus_playerTick(u8 playerIndex)
{
    Actor *actor = PLAYER(playerIndex).ownerActor;

    if (playerIndex >= 2 || actor == NULL || !duelRunning())
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

bool32 versus_isDuelFoe(Actor *actor)
{
    u8 target;

    if (actor == NULL || Duel.acting >= 2 || !duelRunning() || Duel.phase != VERSUS_PHASE_FIGHT)
    {
        return FALSE;
    }
    target = actor->as.combat.playerIndex;
    return target < 2 && target != Duel.acting && actor == PLAYER(target).ownerActor &&
           PLAYER(target).currentHp > 0 && Duel.downFrames[target] == 0 &&
           Duel.shieldFrames[target] == 0 && !(actor->flags.p & ACTOR_FLAG_INVULNERABLE);
}

HackLibDamageResult versus_skillHitsPlayer(Actor *victim, u16 damage)
{
    u8 target = victim->as.combat.playerIndex;
    s32 hpBefore;
    s32 cap;
    u8 previousAttacker = Duel.pvpAttacker;

    if (!versus_isDuelFoe(victim) || damage == 0)
    {
        return (HackLibDamageResult){0};
    }
    hpBefore = PLAYER(target).currentHp;
    cap = PLAYER_STAT(target, STAT_MAX_HP) * VERSUS_SKILL_HIT_MAX_PERCENT / 100;
    // Marked as a hit between players, so the duel scales it like a melee hit.
    Duel.pvpAttacker = Duel.acting;
    Duel.pvpHits++;
    player_subtractHp(target, damage < cap ? damage : (cap > 0 ? cap : 1), TRUE);
    Duel.pvpHits--;
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
