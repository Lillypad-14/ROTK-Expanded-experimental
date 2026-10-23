#include "patches/skill_expansion/skill_expansion.h"
#include "game.h"
#include "gba_io.h"
#include "gfx.h"

// HUD skill panel icon colors. Page 2 rows reuse stock icons, so the HUD loads a page 2 art ramp
// into the panel's palette bank while one of them is selected.

#define ART_COLOR(r, g, b) ((((b) >> 3) << 10) | (((g) >> 3) << 5) | ((r) >> 3))

// Blue: castable page 2 skill, and page 2 tiles in the skill tree.
const u16 SkillExpansionPage2ArtRamp[SKILL_EXPANSION_ICON_ART_COLOR_COUNT] = {
    ART_COLOR(200, 208, 248), ART_COLOR(136, 152, 240), ART_COLOR(96, 104, 216),
    ART_COLOR(64, 64, 176),   ART_COLOR(40, 32, 128),   ART_COLOR(16, 8, 64),
};

// Magenta: page 2 skill the HUD can't cast (stock uses red).
const u16 SkillExpansionPage2UncastableArtRamp[SKILL_EXPANSION_ICON_ART_COLOR_COUNT] = {
    ART_COLOR(240, 184, 232), ART_COLOR(216, 144, 208), ART_COLOR(192, 104, 184),
    ART_COLOR(160, 72, 152),  ART_COLOR(120, 40, 112),  ART_COLOR(72, 8, 64),
};

// Blue-grey: cycle-disabled page 2 tile in the skill tree (stock uses grey).
const u16 SkillExpansionPage2DisabledArtRamp[SKILL_EXPANSION_ICON_ART_COLOR_COUNT] = {
    ART_COLOR(224, 232, 248), ART_COLOR(192, 200, 224), ART_COLOR(160, 168, 192),
    ART_COLOR(128, 136, 160), ART_COLOR(96, 104, 128),  ART_COLOR(64, 72, 96),
};

// Build ramps, castable then uncastable (darker), indexed by SkillExpansionBuild - 1.
static const u16 BuildArtRamps[6][2][SKILL_EXPANSION_ICON_ART_COLOR_COUNT] = {
    {
        {ART_COLOR(248, 240, 200), ART_COLOR(240, 216, 136), ART_COLOR(224, 184, 80),
         ART_COLOR(184, 136, 40), ART_COLOR(128, 88, 16), ART_COLOR(64, 40, 8)},
        {ART_COLOR(160, 152, 128), ART_COLOR(144, 128, 88), ART_COLOR(120, 96, 48),
         ART_COLOR(96, 72, 24), ART_COLOR(64, 40, 8), ART_COLOR(32, 16, 0)},
    },
    {
        {ART_COLOR(216, 176, 240), ART_COLOR(176, 120, 224), ART_COLOR(136, 72, 192),
         ART_COLOR(96, 40, 152), ART_COLOR(64, 16, 104), ART_COLOR(32, 0, 56)},
        {ART_COLOR(136, 112, 152), ART_COLOR(112, 80, 136), ART_COLOR(88, 48, 112),
         ART_COLOR(64, 24, 88), ART_COLOR(40, 8, 56), ART_COLOR(16, 0, 24)},
    },
    {
        {ART_COLOR(200, 248, 184), ART_COLOR(144, 224, 128), ART_COLOR(96, 192, 80),
         ART_COLOR(56, 152, 48), ART_COLOR(32, 104, 24), ART_COLOR(8, 56, 8)},
        {ART_COLOR(128, 152, 120), ART_COLOR(96, 136, 88), ART_COLOR(64, 112, 56),
         ART_COLOR(40, 88, 32), ART_COLOR(24, 56, 16), ART_COLOR(8, 24, 8)},
    },
    {
        {ART_COLOR(216, 248, 240), ART_COLOR(160, 232, 216), ART_COLOR(104, 200, 184),
         ART_COLOR(64, 152, 144), ART_COLOR(32, 96, 96), ART_COLOR(8, 48, 48)},
        {ART_COLOR(136, 160, 152), ART_COLOR(104, 136, 128), ART_COLOR(72, 112, 104),
         ART_COLOR(48, 88, 80), ART_COLOR(24, 56, 56), ART_COLOR(8, 24, 24)},
    },
    {
        {ART_COLOR(240, 240, 240), ART_COLOR(200, 200, 208), ART_COLOR(160, 160, 168),
         ART_COLOR(120, 120, 128), ART_COLOR(80, 80, 88), ART_COLOR(40, 40, 48)},
        {ART_COLOR(144, 144, 144), ART_COLOR(120, 120, 124), ART_COLOR(96, 96, 100),
         ART_COLOR(72, 72, 76), ART_COLOR(48, 48, 52), ART_COLOR(24, 24, 28)},
    },
    {
        {ART_COLOR(248, 224, 168), ART_COLOR(248, 168, 88), ART_COLOR(224, 104, 40),
         ART_COLOR(176, 56, 16), ART_COLOR(112, 24, 8), ART_COLOR(56, 8, 0)},
        {ART_COLOR(160, 136, 112), ART_COLOR(144, 104, 72), ART_COLOR(120, 72, 40),
         ART_COLOR(88, 40, 16), ART_COLOR(56, 16, 8), ART_COLOR(24, 8, 0)},
    },
};

const u16 *skillExpansion_buildArtRamp(u8 build, bool32 uncastable)
{
    if (build == SKILL_EXPANSION_BUILD_NONE || build > SKILL_EXPANSION_BUILD_FIRE)
    {
        return uncastable ? SkillExpansionPage2UncastableArtRamp : SkillExpansionPage2ArtRamp;
    }
    return BuildArtRamps[build - 1][uncastable ? 1 : 0];
}

// Art ramps of the stock HUD palettes (0x087815d4 castable, 0x08782330 uncastable).
static const u16 StockArtRamp[SKILL_EXPANSION_ICON_ART_COLOR_COUNT] = {
    0xdfb6, 0xc32c, 0x32a6, 0xade4, 0x2122, 0x9c20,
};
static const u16 StockUncastableArtRamp[SKILL_EXPANSION_ICON_ART_COLOR_COUNT] = {
    0x5efe, 0xce7b, 0x3a19, 0xa997, 0x9915, 0x8430,
};

// BG palette bank of the HUD skill panel icon.
#define HUD_SKILL_ICON_PALETTE 1
#define PALETTE_COLOR_COUNT 16
#define COLOR_MASK 0x7fff

// Stock PlayerHud.flags bit: the uncastable palette is loaded.
#define PLAYERHUD_FLAG_UNCASTABLE_PALETTE (1 << 9)

/**
 * Replaces player_updateHudSkillPalette. Stock loads the red palette while the selected skill
 * can't be cast and the green one otherwise, tracked by a HUD flag. This picks between four art
 * ramps and compares against palette RAM instead, so a page 2 <-> page 1 switch in the same
 * castable state (or a scene that reloaded the HUD palette) is picked up too.
 */
void skillExpansion_hudUpdateSkillPalette(u8 playerIndex)
{
    const u16 *ramp;
    volatile u16 *paletteRam;
    PlayerHudFlags *flags;
    bool32 canCast;
    bool32 uncastable;
    bool32 page2;
    u32 row;
    u32 i;

    if (PLAYER(playerIndex).optionFlags.p & PLAYER_OPTION_FLAG_QUICK_SKILL_SELECT)
    {
        row = PLAYER(playerIndex)
                  .quickSelectActiveSkills[(s8)PLAYER(playerIndex).selectedActiveSkill.quickSlot];
    }
    else
    {
        row = (s8)PLAYER(playerIndex).selectedActiveSkill.cycleSkillIndex;
    }
    canCast = skill_active_canCast(playerIndex, row);

    if (playerIndex != ACTIVE_PLAYER_INDEX || g_GameFlags.d.overlay)
    {
        return;
    }

    flags = &g_PlayerHuds[playerIndex].flags;
    uncastable = !canCast && !(flags->p & PLAYERHUD_FLAG_UNSPENT_POINTS);
    page2 = (u8)row >= SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW &&
            (u8)row < SKILL_EXPANSION_ACTIVE_ROW_COUNT;

    if (page2)
    {
        ramp = skillExpansion_buildArtRamp(
            skillExpansion_activeBuild(PLAYER(playerIndex).heroId, row), uncastable);
    }
    else
    {
        ramp = uncastable ? StockUncastableArtRamp : StockArtRamp;
    }

    if (uncastable)
    {
        flags->p |= PLAYERHUD_FLAG_UNCASTABLE_PALETTE;
    }
    else
    {
        flags->p &= ~PLAYERHUD_FLAG_UNCASTABLE_PALETTE;
    }

    paletteRam = (volatile u16 *)BG_PALETTE_RAM +
                 HUD_SKILL_ICON_PALETTE * PALETTE_COLOR_COUNT + SKILL_EXPANSION_ICON_ART_FIRST_COLOR;
    for (i = 0; i < SKILL_EXPANSION_ICON_ART_COLOR_COUNT; i++)
    {
        if ((paletteRam[i] & COLOR_MASK) != (ramp[i] & COLOR_MASK))
        {
            gfx_loadBgPaletteColors(ramp,
                                    HUD_SKILL_ICON_PALETTE * PALETTE_COLOR_COUNT +
                                        SKILL_EXPANSION_ICON_ART_FIRST_COLOR,
                                    SKILL_EXPANSION_ICON_ART_COLOR_COUNT);
            return;
        }
    }
}
