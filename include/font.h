#pragma once

#include "text.h"
#include "types.h"

// Serialized font header. Offsets are big-endian and relative to the asset start.
typedef union FontAsset {
    u8 bytes[0];
    struct
    {
        u8 firstChar;
        u8 lastChar;
        u8 height;
        u8 reserved;
        u8 glyphOffsetsOffset[4];
        u8 glyphWidthsOffset[4];
        u8 glyphBitmapsOffset[4];
    } d;
} FontAsset;

typedef struct FontStyleInfo
{
    FontAsset *asset;
    u32 lineHeight; // Default line advance in pixels.
} FontStyleInfo;

/** Decoded font metrics and column-major 2bpp glyph data. */
typedef struct FontDesc
{
    u8 height;
    u8 firstChar;
    u8 lastChar;
    u8 lineHeight;     // Default line advance in pixels.
    u16 *glyphOffsets; // Per-glyph byte offsets into glyphBitmaps.
    u8 *glyphWidths;   // Pixel widths. Glyph 1 is used for unsupported characters.
    u8 *glyphBitmaps;  // Packed, column-major 2bpp samples.
} FontDesc;

/** Current font, colors, and BG render target. */
typedef struct FontRenderCtx
{
    FontDesc *fontDesc;
    s32 colorIndex;      // Palette-index bias for nonzero glyph samples.
    s32 backgroundColor; // Fill color, or -1 to preserve tile pixels and palettes.
    s32 paletteBits;     // Palette-bank bits for 4bpp screen entries.
    s32 lineHeight;      // Line advance in pixels.
    u16 *screenbase;     // Screen-entry base address.
    u16 *screenbaseCopy; // Copy of the screen-entry base pointer.
    u8 *charbase;        // Tile-bitmap base address.
    s32 tileStride;      // Bytes per tile: 32 for 4bpp, 64 for 8bpp.
    bool32 colors256;    // TRUE for 8bpp, FALSE for 4bpp
    s32 field_28;
    void *tileScratch; // 0x800-byte heap buffer
    bool32 dirty;
} FontRenderCtx;

/** Font style, named by glyph height in pixels. */
typedef enum FontStyle
{
    FONT_STYLE_8PX_A = 0,
    FONT_STYLE_8PX_B = 1,
    FONT_STYLE_11PX_A = 2,
    FONT_STYLE_11PX_B = 3,
    FONT_STYLE_21PX = 4,
} FontStyle;

void font_setStyle(FontStyle styleIndex, s32 color, s32 backgroundColor);
void font_setLineHeight(s32 lineHeight);

void font_init(void);
void font_setRenderTarget(u16 *screenbase, u8 *charbase, bool32 colors256);
void font_flipTileHorizontal(u8 *tile);

/**
 * NUL-terminated replacement text for '@1'..'@4'.
 * Must be nonempty and contain no placeholders.
 */
typedef char FontTemplateArg[0x40];

/** Placeholder digit '1'..'4' to zero-based g_FontTemplateArgs index, without range checking. */
#define FONT_TEMPLATE_ARG_INDEX(chr) (CHAR_TO_DIGIT(chr) - 1)

u32 font_getStringWidth(char *str);
u32 font_getStringHeight(char *str);
void font_loadStyleDescriptor(FontStyle styleIndex);
s32 font_getGlyphWidth(FontDesc *fontDesc, s32 ch);

#define FONT_LINE_BUFFER_SIZE 0x40

s32 font_drawLine(s32 charbaseTileIndex, s32 x, s32 y, s32 maxWidth, char **pstr, s32 alignment,
                  u16 *widthOut);
s32 font_drawLinesInBox(s32 charbaseTileIndex, s32 x, s32 y, s32 maxWidth, s32 maxHeight,
                        char **pstr, s32 alignment);
s32 font_drawLines(s32 charbaseTileIndex, s32 x, s32 y, s32 maxWidth, char *str, s32 alignment);
void font_fillScreenRow(s32 colStart, s32 row, s32 colEnd, u16 entry);
void font_clearScreenRow(s32 colStart, s32 row, s32 colEnd);
void font_fillTiles(u32 *tiles, u32 tileCount, u32 color);
void font_readCellColumn(u32 *dst, u32 rows, const u16 *screenEntry, const u8 *charbase);
void font_writeCellColumn(const u32 *src, u32 rows, u16 *screenEntry, u8 *charbase, s32 tileIndex);
u16 *font_screenEntryAt(u16 *screenbase, s32 x, s32 y);

s32 font_drawString(s32 charbaseTileIndex, s32 x, s32 y, char *str, s32 alignment);
s32 font_rasterizeString(s32 charbaseTileIndex, s32 x, s32 y, char *str);

void font_setIntTemplateArg(s32 value, s32 argIndex);
void font_setIntTemplateArg0(s32 value);
void font_setIntTemplateArg1(s32 value);
void font_setIntTemplateArg2(s32 value);

void font_setBgTarget(u32 bgcnt);

void font_setTemplateArg(char *src, s32 argIndex);
void font_setTemplateArg0(char *src);
void font_setTemplateArg1(char *src);
void font_setTemplateArg2(char *src);
void font_setTemplateArg3(char *src);
