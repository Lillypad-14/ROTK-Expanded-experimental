#if !HACK_skill_expansion || !HACK_mod_save
#error "legacy_skills requires skill_expansion and mod_save"
#endif

#include "actor.h"
#include "combat.h"
#include "fx.h"
#include "memory.h"
#include "patches/legacy_skills/legacy_skills.h"
#include "patches/mod_save/save_layout.h"
#include "patches/mod_save/save_slots.h"
#include "patches/mod_save/save_sram.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "lib/pc_host.h"
#include "actor/shieldBubble.h"
#include "player.h"
#include "sfx.h"
#include "spriteAnimation.h"
#include "scene.h"
#include "scene/saveLoadMenu.h"
#include "skill.h"
#include "stats.h"
#include "text.h"
#include "variables.h"

#define LEGACY_SKILL_RECORD_TAG 0x01
#define LEGACY_SKILL_COUNT 11
#define LEGACY_PASSIVE_STATS 2
#define LEGACY_HERO_COUNT 6
#define LEGACY_PASSIVE_MAX 6
#define LEGACY_SKILL_LEVEL_MAX 20
#define LEGACY_SKILL_RECORD_EMPTY 0xff

// One stat a legacy passive adds: base + perLevel * level / divisor at levels 1 and up.
typedef struct LegacyPassiveStat
{
    u8 stat;
    s8 base;
    s8 perLevel;
    u8 divisor;
} LegacyPassiveStat;

// A hero's legacy passives, in the skill record's order (they come first).
typedef struct LegacyHeroPassives
{
    u8 count;
    LegacyPassiveStat stats[LEGACY_PASSIVE_MAX][LEGACY_PASSIVE_STATS];
} LegacyHeroPassives;

#define NONE {STAT_NONE, 0, 0, 1}
#define PER_LEVEL(stat, perLevel) {stat, 0, perLevel, 1}
// value at level 1 is `first`, each further level adds `step`
#define FROM(stat, first, step) {stat, (first) - (step), step, 1}

// The Two Towers passives by hero id. A skill with no ROTK mechanic uses the closest stat.
static const LegacyHeroPassives HeroPassives[LEGACY_HERO_COUNT] = {
    // Frodo: Resist One Ring, Strength of the Shire, Rummage, Sneak Attack, Duck
    {5,
     {{PER_LEVEL(STAT_CORRUPTION_RESIST_PERCENT, 4), NONE},
      {PER_LEVEL(STAT_HP_REGEN, 1), PER_LEVEL(STAT_SPIRIT_REGEN, 1)},
      {PER_LEVEL(STAT_EXTRA_TREASURE_PERCENT, 10), NONE},
      {PER_LEVEL(STAT_BACKSTAB, 2), NONE},
      {PER_LEVEL(STAT_MELEE_ARMOR, 1), PER_LEVEL(STAT_MISSILE_ARMOR, 1)}}},
    // Legolas: Meditation, Fleet of Foot, Orc Slayer, Hawk Eye, Critical Shot
    {5,
     {{PER_LEVEL(STAT_MEDITATIVE, 1), NONE},
      {PER_LEVEL(STAT_SPEED_PERCENT, 3), NONE},
      {PER_LEVEL(STAT_DAMAGE_TO_ORCS, 2), NONE},
      {PER_LEVEL(STAT_DAMAGE_WITH_BOW, 1), NONE},
      {FROM(STAT_CRIT_CHANCE_PERCENT, 10, 2), NONE}}},
    // Aragorn: Dual Wield, Numenorean Blood, Shield Offense, Parry, Ferocity
    {5,
     {{PER_LEVEL(STAT_ARAGORN_DUAL_WIELD_DAMAGE, 1), NONE},
      {PER_LEVEL(STAT_CORRUPTION_RESIST_PERCENT, 5), PER_LEVEL(STAT_DAMAGE_TO_NAZGUL, 2)},
      {{STAT_DAMAGE_WITH_SHIELD, 0, 1, 2}, PER_LEVEL(STAT_BLOCK_PERCENT, 1)},
      {FROM(STAT_PARRY_PERCENT, 5, 1), NONE},
      {FROM(STAT_HP_PERCENT_PER_KILL, 5, 1), NONE}}},
    // Gandalf: Elusion, Inspire, Wisdom, Flame of Anor
    {4,
     {{FROM(STAT_CORRUPTION_RESIST_PERCENT, 20, 4), NONE},
      {PER_LEVEL(STAT_SPIRIT_REGEN_PERCENT, 25), NONE},
      {PER_LEVEL(STAT_WISDOM, 1), NONE},
      {FROM(STAT_REVIVE_CHANCE_PERCENT, 20, 2), NONE}}},
    // Eowyn: Strength of Heart, Iron Will, Stand Fast, Eyes of Rohan, Ferocity
    {5,
     {{PER_LEVEL(STAT_CORRUPTION_RESIST_PERCENT, 5), PER_LEVEL(STAT_DAMAGE_TO_NAZGUL, 2)},
      {PER_LEVEL(STAT_HP_REGEN, 1), NONE},
      {FROM(STAT_PARRY_PERCENT, 5, 1), NONE},
      {PER_LEVEL(STAT_EXTRA_TREASURE_PERCENT, 10), NONE},
      {PER_LEVEL(STAT_SPIRIT_PER_KILL, 1), NONE}}},
    // Gimli: Strength of Beard, Orcslayer, Headsplitter, Gemfinder, Dwarven Resilience, Spirit of
    // the Forge
    {6,
     {{PER_LEVEL(STAT_MAX_HP, 10), NONE},
      {PER_LEVEL(STAT_DAMAGE_TO_ORCS, 2), NONE},
      {FROM(STAT_CRIT_CHANCE_PERCENT, 10, 3), NONE},
      {PER_LEVEL(STAT_GEMFINDER, 1), NONE},
      {FROM(STAT_DODGE_PERCENT, 10, 2), NONE},
      {PER_LEVEL(STAT_SPIRIT_REGEN, 1), NONE}}},
};

// The save slot the save menu has selected, else the one the player loaded.
static u32 currentSlot(void)
{
    u32 slot = g_SceneCurrent.id == SCENE_ID_SAVE_LOAD_MENU ? SCENE_ARG_SAVE_LOAD_SELECTED_SLOT
                                                             : g_LastSaveSlot;

    return g_SaveSlotPage * SAVE_SLOTS_PER_PAGE + slot;
}

static u8 *recordAddress(u32 slotIndex)
{
    return SRAM_BASE + LEGACY_SKILL_SAVE_OFF + slotIndex * LEGACY_SKILL_SAVE_RECORD_SIZE;
}

// The legacy skill record of the slot the player loaded, FALSE when it has none.
static bool32 readRecord(u8 record[LEGACY_SKILL_SAVE_RECORD_SIZE])
{
    modSave_copy(record, recordAddress(currentSlot()), LEGACY_SKILL_SAVE_RECORD_SIZE);
    return record[0] == LEGACY_SKILL_RECORD_TAG;
}

void legacySkills_applyPassives(u8 playerIndex)
{
    u8 record[LEGACY_SKILL_SAVE_RECORD_SIZE];
    const LegacyHeroPassives *passives;

    // A co-op partner's record stays on the other console.
    if (playerIndex != 0 || g_PlayerGlobals.activePlayerCount > 1 || !readRecord(record) ||
        record[1] != PLAYER(playerIndex).heroId || record[1] >= LEGACY_HERO_COUNT)
    {
        return;
    }
    passives = &HeroPassives[record[1]];
    for (u32 skill = 0; skill < passives->count; skill++)
    {
        s32 level = record[2 + skill];
        if (level == 0 || level > LEGACY_SKILL_LEVEL_MAX)
        {
            continue;
        }
        for (u32 i = 0; i < LEGACY_PASSIVE_STATS; i++)
        {
            const LegacyPassiveStat *stat = &passives->stats[skill][i];
            if (stat->stat != STAT_NONE)
            {
                skill_passive_addStat(playerIndex, stat->stat,
                                      stat->base + stat->perLevel * level / stat->divisor);
            }
        }
    }
}

#if SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

// A legacy active: its row's definition (the stock row it takes its cast pose and table row from)
// and its Spirit cost in The Two Towers. It casts `cast`, a port of the Two Towers skill, or else
// the definition's stock callback, a ROTK skill standing in.
typedef struct LegacyActive
{
    const SkillExpansionActiveDef *def;
    u16 spiritCost;
    ActiveSkillCallback cast;
    // Spirit the cost adds per level
    u8 spiritPerLevel;
} LegacyActive;

// A Two Towers animation the PC host converted, by its Two Towers address, or NULL.
static const SpriteAnimation *twoTowersSprite(u32 address)
{
    const TwoTowersData *data = hackLib_twoTowersData();

    if (data == NULL || data->sprites == NULL)
    {
        return NULL;
    }
    for (u32 i = 0; i < data->spriteCount; i++)
    {
        if (data->sprites[i * 2] == address)
        {
            return (const SpriteAnimation *)data->sprites[i * 2 + 1];
        }
    }
    return NULL;
}

/**
 * Play a Two Towers animation on an effect. The spawner leaves its own animation script running,
 * which would drive the frames and keep the animation from ever ending.
 */
static void setSprite(Actor *fx, u32 animation)
{
    const SpriteAnimation *sprite = twoTowersSprite(animation);

    if (sprite != NULL)
    {
        fx->animationScriptCursor = NULL;
        sprite_setAnimation(fx, sprite);
    }
}

static u8 castingLevel(Actor *playerActor)
{
    u8 playerIndex = playerActor->as.combat.playerIndex;

    return *skillExpansion_activeLevel(playerIndex, PLAYER(playerIndex).castingActiveSkill);
}

// Zap (Gandalf; Two Towers 0x08018FD4, its tick 0x080191F0, hit 0x080115E0): a bolt that flies
// for 30 frames and bursts on the first foe it hits for 5 + 2 per level.
#define ZAP_DAMAGE_BASE 5
#define ZAP_DAMAGE_PER_LEVEL 2
#define ZAP_SPEED 0x50000
#define ZAP_LIFETIME 30
#define ZAP_IMPACT 0x080501C8u
// the bolt by facing
static const u32 ZapSprites[8] = {
    0x08050138u, 0x08050148u, 0x08050148u, 0x08050148u,
    0x08050158u, 0x08050158u, 0x08050168u, 0x08050168u,
};
// the projectile spawner's arguments for a thrown knife: no target, its palette bank, no slot
#define PROJECTILE_ANIMATION_SET 1
#define PROJECTILE_AIMED 1
#define PROJECTILE_PALETTE_BANK 2
#define PROJECTILE_NO_SLOT -1
#define FOE_HITTABLE_FLAGS 0x10040400
// Lightstrike's cast sound until the Two Towers sounds are ported
#define ZAP_SOUND 0x5a

static void zapTick(Actor *fx)
{
    // the impact plays out where it hit, then the bolt is gone
    if (fx->triggerCallbacks[0] == NULL)
    {
        fx->actionStateTimer = (fx->flags.p & ACTOR_FLAG_ANIMATION_END) ? 0 : ZAP_LIFETIME;
    }
    if (fx->parentActor->actionState == ACTOR_STATE_UNKNOWN_2D)
    {
        fx->actionState = ACTOR_STATE_UNKNOWN_04;
    }
    actor_unknown08045584(fx);
}

static void zapHit(Actor *fx, Actor *other)
{
    if (!(other->flags.p & FOE_HITTABLE_FLAGS))
    {
        return;
    }
    combat_applyPveRangedHit(fx->parentActor, other, *(u16 *)&fx->as.combat.head.scripted.params[0]);
    fx->triggerCallbacks[0] = NULL;
    fx->renderFollowTarget = other;
    fx->velocity.x = 0;
    fx->velocity.y = 0;
    setSprite(fx, ZAP_IMPACT);
}

// An effect that plays `animation` on the caster once, then is gone.
static void effectTick(Actor *fx)
{
    fx->coords = fx->parentActor->coords;
    fx->actionStateTimer = (fx->flags.p & ACTOR_FLAG_ANIMATION_END) ? 0 : ZAP_LIFETIME;
    actor_unknown08045584(fx);
}

static void spawnEffectOn(Actor *playerActor, u32 animation)
{
    Actor *fx;

    if (twoTowersSprite(animation) == NULL)
    {
        return;
    }
    fx = fx_unknown08043d74(playerActor, 0, PROJECTILE_ANIMATION_SET, PROJECTILE_AIMED,
                            PROJECTILE_PALETTE_BANK, PROJECTILE_NO_SLOT);
    if (fx == NULL)
    {
        return;
    }
    setSprite(fx, animation);
    fx->velocity.x = 0;
    fx->velocity.y = 0;
    fx->triggerCallbacks[0] = NULL;
    fx->parentActor = playerActor;
    fx->tickCallback = effectTick;
}

// Magical Healing (Gandalf; Two Towers 0x0803C4F8, 0x0801EA40): heals 20 + 4 per level for 8 + 2
// per level Spirit, with a sparkle on Gandalf.
#define MAGICAL_HEALING_BASE 20
#define MAGICAL_HEALING_PER_LEVEL 4
#define MAGICAL_HEALING_SPARKLE 0x080501D8u
// the herb pickup sound until the Two Towers sounds are ported
#define MAGICAL_HEALING_SOUND SFX_HERB_PICKUP

static void castMagicalHealing(Actor *playerActor)
{
    u8 playerIndex = playerActor->as.combat.playerIndex;

    player_addHp(playerIndex, MAGICAL_HEALING_BASE + MAGICAL_HEALING_PER_LEVEL * castingLevel(playerActor),
                 FALSE);
    spawnEffectOn(playerActor, MAGICAL_HEALING_SPARKLE);
    sfx_play(MAGICAL_HEALING_SOUND);
}

// Shield (Gandalf; Two Towers 0x0803BB1C, 0x080185B4, its tick 0x0801871C): a bubble on Gandalf
// that absorbs 50 + 20 per level damage for 750 frames, for 10 Spirit and 2 corruption per level.
// One at a time. It absorbs through ROTK's own shield bubble (PLAYER().shieldBubble).
#define SHIELD_ABSORB_BASE 50
#define SHIELD_ABSORB_PER_LEVEL 20
#define SHIELD_FRAMES 750
#define SHIELD_CORRUPTION_PER_LEVEL 2
#define SHIELD_FORMING 0x080500C8u
#define SHIELD_UP 0x080500D8u
#define SHIELD_DISSOLVING 0x080500E8u
// the shield's phase, kept in its charge level (Gimli's shields only use that)
#define SHIELD_PHASE_FORMING 0
#define SHIELD_PHASE_UP 1
#define SHIELD_PHASE_DISSOLVING 2

static bool32 shieldCanCast(u8 playerIndex)
{
    return PLAYER(playerIndex).shieldBubble == NULL;
}

static void shieldTick(Actor *fx)
{
    Actor *playerActor = fx->parentActor;
    u8 playerIndex = playerActor->as.combat.playerIndex;
    s16 *phase = &fx->as.shieldGandalf.chargeLevel;
    bool32 ended = (fx->flags.p & ACTOR_FLAG_ANIMATION_END) != 0;

    fx->coords = playerActor->coords;
    // an absorbed hit flashes the bubble in ROTK; it just stays up here
    if (fx->actionState == ACTOR_STATE_SHIELD_SPENT)
    {
        fx->actionState = ACTOR_STATE_FX_ATTACHED;
    }
    if (*phase == SHIELD_PHASE_FORMING && ended)
    {
        *phase = SHIELD_PHASE_UP;
        fx->actionState = ACTOR_STATE_FX_ATTACHED;
        setSprite(fx, SHIELD_UP);
    }
    else if (*phase == SHIELD_PHASE_UP &&
             (fx->as.shieldGandalf.shieldHealth <= 0 || --fx->actionStateTimer < 0))
    {
        *phase = SHIELD_PHASE_DISSOLVING;
        setSprite(fx, SHIELD_DISSOLVING);
    }
    else if (*phase == SHIELD_PHASE_DISSOLVING && ended)
    {
        fx->actionState = ACTOR_STATE_UNKNOWN_04;
    }
    if (playerActor->actionState == ACTOR_STATE_UNKNOWN_2D)
    {
        fx->actionState = ACTOR_STATE_UNKNOWN_04;
    }
    if (fx->actionState == ACTOR_STATE_UNKNOWN_04)
    {
        if (PLAYER(playerIndex).shieldBubble == fx)
        {
            PLAYER(playerIndex).shieldBubble = NULL;
        }
        fx->actionStateTimer = 0;
        actor_unknown08045584(fx);
    }
}

static void castShield(Actor *playerActor)
{
    u8 playerIndex = playerActor->as.combat.playerIndex;
    u8 level = castingLevel(playerActor);
    Actor *fx;

    player_addCorruption(playerIndex, SHIELD_CORRUPTION_PER_LEVEL * level);
    fx = fx_unknown08043d74(playerActor, 0, PROJECTILE_ANIMATION_SET, PROJECTILE_AIMED,
                            PROJECTILE_PALETTE_BANK, PROJECTILE_NO_SLOT);
    if (fx == NULL)
    {
        return;
    }
    setSprite(fx, SHIELD_FORMING);
    fx->velocity.x = 0;
    fx->velocity.y = 0;
    fx->triggerCallbacks[0] = NULL;
    fx->parentActor = playerActor;
    fx->tickCallback = shieldTick;
    fx->actionStateTimer = SHIELD_FRAMES;
    fx->as.shieldGandalf.followTarget = playerActor;
    fx->as.shieldGandalf.shieldHealth = SHIELD_ABSORB_BASE + SHIELD_ABSORB_PER_LEVEL * level;
    fx->as.shieldGandalf.chargeLevel = SHIELD_PHASE_FORMING;
    PLAYER(playerIndex).shieldBubble = fx;
    sfx_play(SFX_COMBAT_GANDALF_SHIELD_CAST);
}

static void castZap(Actor *playerActor)
{
    u8 level = castingLevel(playerActor);
    Actor *fx = fx_unknown08043d74(playerActor, 0, PROJECTILE_ANIMATION_SET, PROJECTILE_AIMED,
                                   PROJECTILE_PALETTE_BANK, PROJECTILE_NO_SLOT);
    if (fx == NULL)
    {
        return;
    }
    setSprite(fx, ZapSprites[fx->facing & 7]);
    actor_setVelocityFromDirection8(&fx->velocity, fx->flags.p, fx->facing, ZAP_SPEED);
    fx->actionStateTimer = ZAP_LIFETIME;
    fx->tickCallback = zapTick;
    fx->triggerCallbacks[0] = zapHit;
    fx->parentActor = playerActor;
    *(u16 *)&fx->as.combat.head.scripted.params[0] = ZAP_DAMAGE_BASE + ZAP_DAMAGE_PER_LEVEL * level;
    sfx_play(ZAP_SOUND);
}

// Lightstrike returns its bolt, which the cast does not need.
static void castLightstrike(Actor *playerActor)
{
    skill_active_gandalfLightstrike(playerActor);
}

#define LEGACY_DEF(name, callback, base)                                                               static const SkillExpansionActiveDef name = {                                                          .stockCallback = (callback),                                                  .baseRow = (base),                                                                             }

LEGACY_DEF(LegacyGandalfLightstrike, castLightstrike, ACTIVE_SKILL_GANDALF_LIGHTSTRIKE);
// a port: Lightstrike's cast pose and requirements, its own cast
static const SkillExpansionActiveDef LegacyGandalfZap = {.baseRow = ACTIVE_SKILL_GANDALF_LIGHTSTRIKE};
static bool32 shieldCanCast(u8 playerIndex);
static const SkillExpansionActiveDef LegacyGandalfShieldPort = {
    .baseRow = ACTIVE_SKILL_GANDALF_SHIELD,
    .canCast = shieldCanCast,
};
// Shield's pose: casting through Herbal Healing's row hangs the game
static const SkillExpansionActiveDef LegacyGandalfHealing = {.baseRow = ACTIVE_SKILL_GANDALF_SHIELD};
LEGACY_DEF(LegacyGandalfBlindingAura, skill_active_gandalfBlindingAura,
           ACTIVE_SKILL_GANDALF_BLINDING_AURA);
LEGACY_DEF(LegacyGandalfGwaihir, skill_active_gandalfSummonGwaihir,
           ACTIVE_SKILL_GANDALF_SUMMON_GWAIHIR);

// The Two Towers actives by hero id, in the skill record's order (after the passives). NULL: none
// yet.
static const LegacyActive HeroActives[LEGACY_HERO_COUNT][SKILL_EXPANSION_LEGACY_ACTIVE_COUNT] = {
    [3] =
        {
            {&LegacyGandalfHealing, 8, castMagicalHealing, 2}, // Magical Healing
            {&LegacyGandalfShieldPort, 10, castShield}, // Shield
            {&LegacyGandalfZap, 3, castZap}, // Zap
            {&LegacyGandalfBlindingAura, 8}, // Sunburst
            {&LegacyGandalfLightstrike, 5},  // Fireworks: stand-in until it has its own cast
            {&LegacyGandalfGwaihir, 12},     // Summon Eagle
            {&LegacyGandalfBlindingAura, 20}, // Thunderstrike: stand-in until it has its own cast
        },
};

// The legacy skill record of the character player 0 plays, record[0] != tag when it has none.
static u8 loadedRecord[LEGACY_SKILL_SAVE_RECORD_SIZE];

// A ROTK skill icon: a BG asset header (raw 4bpp tiles, no palette) and 4x3 tiles.
#define SKILL_ICON_HEADER_SIZE 4
#define SKILL_ICON_WIDTH 32
#define SKILL_ICON_HEIGHT 24
#define SKILL_ICON_TILE_BYTES (SKILL_ICON_WIDTH * SKILL_ICON_HEIGHT / 2)
#define SKILL_ICON_RAW_TILES 0x10
#define SKILL_ICON_FRAME_COLOR_LAST 7

typedef struct LegacySkillIcon
{
    u8 header[SKILL_ICON_HEADER_SIZE];
    u8 tiles[SKILL_ICON_TILE_BYTES];
} LegacySkillIcon;

// The loaded hero's Two Towers skill icons, valid while iconsBuilt.
static LegacySkillIcon icons[LEGACY_SKILL_COUNT];
static bool iconsBuilt;

static u8 *iconPixelByte(u8 *tiles, u32 x, u32 y)
{
    return &tiles[((y / 8) * (SKILL_ICON_WIDTH / 8) + x / 8) * 32 + (y % 8) * 4 + (x % 8) / 2];
}

/**
 * Draw the hero's Two Towers skill icons in ROTK's style: the outer border of a stock icon (`frame`,
 * raw tiles) around the Two Towers art the PC host converted.
 */
static void buildIcons(u32 heroId, const u8 *frame)
{
    const TwoTowersData *data = hackLib_twoTowersData();

    iconsBuilt = FALSE;
    if (data == NULL || data->skillArt == NULL || frame == NULL || frame[0] != SKILL_ICON_RAW_TILES)
    {
        return;
    }
    for (u32 skill = 0; skill < LEGACY_SKILL_COUNT; skill++)
    {
        const u8 *art = data->skillArt + (heroId * LEGACY_SKILL_COUNT + skill) * SKILL_ICON_TILE_BYTES;
        LegacySkillIcon *icon = &icons[skill];

        memory_memcpy(icon->header, frame, SKILL_ICON_HEADER_SIZE);
        memory_memcpy(icon->tiles, art, SKILL_ICON_TILE_BYTES);
        for (u32 y = 0; y < SKILL_ICON_HEIGHT; y++)
        {
            for (u32 x = 0; x < SKILL_ICON_WIDTH; x++)
            {
                u32 shift = (x & 1) * 4;
                u8 border = (*iconPixelByte((u8 *)frame + SKILL_ICON_HEADER_SIZE, x, y) >> shift) & 0xf;
                u8 *at = iconPixelByte(icon->tiles, x, y);

                if ((x <= 1 || x >= SKILL_ICON_WIDTH - 2 || y == 0 || y == SKILL_ICON_HEIGHT - 1) &&
                    border != 0 && border <= SKILL_ICON_FRAME_COLOR_LAST)
                {
                    *at = (*at & ~(0xf << shift)) | (border << shift);
                }
            }
        }
    }
    iconsBuilt = TRUE;
}

void legacySkills_buildRows(u8 heroId, u8 playerIndex)
{
    SkillExpansionPlayer *sx = &skillExpansion_players[playerIndex];
    const LegacyActive *actives;
    u32 first;

    for (u32 i = 0; i < SKILL_EXPANSION_LEGACY_ACTIVE_COUNT; i++)
    {
        u32 row = SKILL_EXPANSION_FIRST_LEGACY_ACTIVE_ROW + i;

        memory_memset(&sx->activeSkills[row], 0, sizeof(ActiveSkill));
        sx->activeSkillIcons[row] = NULL;
        sx->activeDefs[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW] = NULL;
        *skillExpansion_activePurchased(playerIndex, row) = 0;
    }
    // A co-op partner's record stays on the other console.
    if (playerIndex != 0)
    {
        return;
    }
    if (!readRecord(loadedRecord) || loadedRecord[1] != heroId || heroId >= LEGACY_HERO_COUNT)
    {
        loadedRecord[0] = LEGACY_SKILL_RECORD_EMPTY;
        return;
    }
    buildIcons(heroId, sx->activeSkillIcons[0]);
    actives = HeroActives[heroId];
    first = 2 + HeroPassives[heroId].count;
    for (u32 i = 0; i < SKILL_EXPANSION_LEGACY_ACTIVE_COUNT && first + i < 2 + LEGACY_SKILL_COUNT;
         i++)
    {
        const SkillExpansionActiveDef *def = actives[i].def;
        u32 row = SKILL_EXPANSION_FIRST_LEGACY_ACTIVE_ROW + i;
        u8 level = loadedRecord[first + i];

        if (def == NULL)
        {
            continue;
        }
        memory_memcpy(&sx->activeSkills[row], &sx->activeSkills[def->baseRow], sizeof(ActiveSkill));
        sx->activeSkills[row].spiritCost =
            actives[i].spiritCost + actives[i].spiritPerLevel * (level > ACTIVE_SKILL_MAX_LEVEL ? ACTIVE_SKILL_MAX_LEVEL : level);
        sx->activeSkills[row].callback =
            actives[i].cast != NULL ? actives[i].cast : skillExpansion_castStockCallback;
        sx->activeSkillIcons[row] = iconsBuilt ? (const void *)&icons[first - 2 + i]
                                               : sx->activeSkillIcons[def->baseRow];
        sx->activeDefs[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW] = def;
        *skillExpansion_activeCycleEnabled(playerIndex, row) = TRUE;
        // The stock curves have five levels.
        *skillExpansion_activePurchased(playerIndex, row) =
            level > LEGACY_SKILL_LEVEL_MAX ? 0 : level > ACTIVE_SKILL_MAX_LEVEL ? ACTIVE_SKILL_MAX_LEVEL : level;
    }
}

void legacySkills_slotCommitted(bool32 saved)
{
    u8 empty[LEGACY_SKILL_SAVE_RECORD_SIZE];
    const u8 *record = loadedRecord;

    if (!saved || loadedRecord[0] != LEGACY_SKILL_RECORD_TAG ||
        loadedRecord[1] != PLAYER(0).heroId)
    {
        memory_memset(empty, LEGACY_SKILL_RECORD_EMPTY, sizeof(empty));
        record = empty;
    }
    modSave_copy(recordAddress(currentSlot()), record, LEGACY_SKILL_SAVE_RECORD_SIZE);
}

// The text of each hero's first skill in The Two Towers; its 11 skills follow in record order.
static const u16 HeroSkillTextIds[LEGACY_HERO_COUNT] = {348, 359, 370, 381, 403, 392};

bool32 legacySkills_loaded(u8 playerIndex)
{
    return playerIndex == 0 && loadedRecord[0] == LEGACY_SKILL_RECORD_TAG &&
           loadedRecord[1] == PLAYER(0).heroId && loadedRecord[1] < LEGACY_HERO_COUNT;
}

u32 legacySkills_passiveCount(void)
{
    return legacySkills_loaded(0) ? HeroPassives[loadedRecord[1]].count : 0;
}

u32 legacySkills_activeCount(void)
{
    return legacySkills_loaded(0) ? LEGACY_SKILL_COUNT - HeroPassives[loadedRecord[1]].count : 0;
}

u8 legacySkills_level(u32 skill)
{
    u8 level = skill < LEGACY_SKILL_COUNT ? loadedRecord[2 + skill] : 0;

    return level > LEGACY_SKILL_LEVEL_MAX ? 0 : level;
}

const void *legacySkills_icon(u32 skill)
{
    return legacySkills_loaded(0) && iconsBuilt && skill < LEGACY_SKILL_COUNT ? &icons[skill] : NULL;
}

const char *legacySkills_text(u32 skill)
{
    const TwoTowersData *data = hackLib_twoTowersData();
    u32 language = text_getLanguage();
    u32 id;

    if (data == NULL || !legacySkills_loaded(0) || skill >= LEGACY_SKILL_COUNT ||
        language >= LANGUAGE_COUNT)
    {
        return NULL;
    }
    id = HeroSkillTextIds[loadedRecord[1]] + skill;
    return id < data->stringCount ? data->strings[language][id] : NULL;
}

#endif
