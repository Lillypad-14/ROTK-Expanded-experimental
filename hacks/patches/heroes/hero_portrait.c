// The character stats screen shows a registered hero's own dialog portrait (PC edition). The
// stock screen draws the hero's portrait asset on BG3; heroes past the stock 8 had their stock
// hero's face. A dialog portrait is 7 x 8 cells; the stats frame shows 8 x 6, so the portrait's
// top and bottom rows are cut and the spare column takes its background color. BG3 is a
// 256-color layer: the portrait's 4bpp tiles are expanded to 8bpp onto its 16 colors in bank 6.
#include "gba_io.h"
#include "gfx.h"
#include "hero_count.h"
#include "patches/heroes/hero_select.h"
#include "player.h"
#include "variables.h"

#if PC_EDITION && HACK_gauntlet && HACK_ringwraith

// The stats screen's portrait layer (BG3, BGCNT 0x1c8d) and its photo area in cells.
#define STATS_BGCNT 0x1c8d
#define PHOTO_X 2
#define PHOTO_Y 13
#define PHOTO_W 8
#define PHOTO_H 6
#define PALETTE_BANK 6
#define FIRST_TILE 1
#define MAP_WIDTH 32

// Dialog portraits: 7 x 8 cells of 4bpp tiles after a 16-color palette.
#define PORTRAIT_W 7
#define PORTRAIT_H 8
#define FILL_TILE (FIRST_TILE + PORTRAIT_W * PORTRAIT_H)
#define TILE_4BPP 32
#define TILE_8BPP 64
// The 4bpp portrait is unpacked past the 8bpp tiles, below BG3's screenbase, then expanded.
#define SCRATCH_OFFSET 0x1600

void heroPortrait_stats(const u8 *stockAsset);

// Called from the stats screen enter in place of its portrait palette load, with r0 the stock
// portrait asset; resumes the screen after that call.
asm(".section .text\n"
    ".balign 4\n"
    ".thumb\n"
    ".global heroPortrait_statsHook\n"
    ".thumb_func\n"
    "heroPortrait_statsHook:\n"
    "    bl   heroPortrait_stats\n"
    "    ldr  r0, =0x080275d3\n"
    "    bx   r0\n"
    ".pool\n");

void heroPortrait_stats(const u8 *stockAsset)
{
    u32 hero = PLAYER(ACTIVE_PLAYER_INDEX).heroId;

    if (hero < 8 || hero >= HACK_HERO_COUNT)
    {
        gfx_loadBgPaletteColors((const u16 *)(stockAsset + 0xc2), PALETTE_BANK * 16, 0xa0);
        return;
    }
    const u8 *asset = HeroMenuInfos[hero].portrait;
    const u16 *palette = (const u16 *)(asset + 2);
    const u8 *stream = asset + 2 + 16 * sizeof(u16) + sizeof(u16);
    u8 *charbase = (u8 *)BG_CHAR_ADDR(BGCNT_GET_CHARBASE(STATS_BGCNT));
    const u8 *packed = charbase + SCRATCH_OFFSET;
    volatile u16 *tiles = (volatile u16 *)(charbase + FIRST_TILE * TILE_8BPP);
    volatile u16 *fill = (volatile u16 *)(charbase + FILL_TILE * TILE_8BPP);
    volatile u16 *map = (volatile u16 *)BG_SCREEN_ADDR(BGCNT_GET_SCREENBASE(STATS_BGCNT));
    u16 background;

    gfx_loadBgPaletteColors(palette, PALETTE_BANK * 16, 16);
    ((void (*)(const void *, void *, u32))g_DecompressPucrunch)(stream, (void *)packed, 0);
    // Each 4bpp byte (two pixels, low nibble first) becomes two 8bpp pixels in bank 6.
    for (u32 i = 0; i < PORTRAIT_W * PORTRAIT_H * TILE_4BPP; i++)
    {
        u8 pair = packed[i];

        tiles[i] = (u16)((PALETTE_BANK * 16 + (pair & 0xf)) | (PALETTE_BANK * 16 + (pair >> 4)) << 8);
    }
    // The portrait's top-left pixel is its background: a solid tile of it fills the spare column.
    background = (u16)((PALETTE_BANK * 16 + (packed[0] & 0xf)) * 0x0101u);
    for (u32 i = 0; i < TILE_8BPP / 2; i++)
    {
        fill[i] = background;
    }
    for (u32 y = 0; y < PHOTO_H; y++)
    {
        for (u32 x = 0; x < PHOTO_W; x++)
        {
            // Rows 1..6 of the portrait: its top and bottom rows are cut.
            u32 tile = x < PORTRAIT_W ? FIRST_TILE + (y + 1) * PORTRAIT_W + x : FILL_TILE;

            map[(PHOTO_Y + y) * MAP_WIDTH + PHOTO_X + x] = (u16)tile;
        }
    }
}

#endif
