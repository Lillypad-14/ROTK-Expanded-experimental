#include "font.h"
#include "gba_io.h"
#include "memory.h"
#include "variables.h"

static inline u32 getStringWidth(char *str)
{
    FontDesc *fontDesc = g_FontRenderCtx.fontDesc;
    s32 width = 0;

    for (; *str != '\0'; str++)
    {
        if (*str == '@')
        {
            str++;
            width += font_getStringWidth(g_FontTemplateArgs[FONT_TEMPLATE_ARG_INDEX(*str)]);
        }
        else
        {
            width += font_getGlyphWidth(fontDesc, *str);
        }
    }
    return width;
}

static inline u32 getStringHeight(char *str)
{
    u32 height = 0;

    if (*str != '\0')
    {
        height = g_FontRenderCtx.fontDesc->height;
    }
    return height;
}

/**
 * Draw one word-wrapped line and advance *pstr past it, a newline, and following spaces.
 * Leave the text pointer and widthOut unchanged if no fitting wrap point exists.
 * Selected lines must contain at most 63 source bytes, including placeholders.
 *
 * @param widthOut receives the drawn line's pixel width when a line is drawn
 *
 * @return next charbase tile index
 *
 * @romaddress 0x0801cf74
 */
s32 font_drawLine(s32 charbaseTileIndex, s32 x, s32 y, s32 maxWidth, char **pstr, s32 alignment,
                  u16 *widthOut)
{
    char line[FONT_LINE_BUFFER_SIZE];
    FontDesc *fontDesc = g_FontRenderCtx.fontDesc;
    char *str = *pstr;
    char *lineEnd = NULL;
    char *hyphenEnd = NULL;
    char *dst;
    s32 width = 0;

    // Measure text and record wrap points until a newline or width overflow.
    for (; *str != '\0' && width <= maxWidth; str++)
    {
        if (*str == '\n')
        {
            lineEnd = str;
            break;
        }

        if (*str == '@')
        {
            // Measure the template argument, not the two-character placeholder.
            str++;
            width += getStringWidth(g_FontTemplateArgs[FONT_TEMPLATE_ARG_INDEX(*str)]);
        }
        else
        {
            if (*str == ' ')
            {
                lineEnd = str;
            }

            width += font_getGlyphWidth(fontDesc, *str);
            if (*str == '-' && width <= maxWidth)
            {
                hyphenEnd = str + 1;
            }
        }
    }

    // Use the whole string if it fits
    if (*str == '\0' && width <= maxWidth)
    {
        lineEnd = str;
    }

    if (lineEnd == NULL)
    {
        if (hyphenEnd == NULL)
        {
            // No fitting breakpoint, don't draw
            return charbaseTileIndex;
        }

        // Use a hyphen wrap point if no other wrap point exists
        lineEnd = hyphenEnd;
    }

    // Copy until chosen line end
    // BUG: Unbounded copy. Lines of 64 or more source bytes overflow the buffer.
    dst = line;
    str = *pstr;
    *pstr = lineEnd;
    while (str != lineEnd)
    {
        *dst++ = *str++;
    }
    *dst = '\0';

    // Render the selected line and report its pixel width.
    charbaseTileIndex = font_drawString(charbaseTileIndex, x, y, line, alignment);
    *widthOut = getStringWidth(line);

    // Advance past the newline and spaces before the next line.
    if (**pstr == '\n')
    {
        (*pstr)++;
    }
    while (**pstr == ' ')
    {
        (*pstr)++;
    }

    return charbaseTileIndex;
}

/**
 * Draw a string at pixel (x, y) with the current font and BG render target.
 *
 * @param alignment 0: left/top, 1: centered on x, 2: right-aligned to x,
 *                  4: centered on y, 8: bottom-aligned to y. Codes do not combine.
 *
 * @return next charbase tile index
 *
 * @romaddress 0x0801d104
 */
s32 font_drawString(s32 charbaseTileIndex, s32 x, s32 y, char *str, s32 alignment)
{
    s32 drawX = x;
    s32 drawY = y;

    if (alignment == 1)
    {
        drawX -= ((s32)getStringWidth(str) + 1) / 2;
    }
    else if (alignment == 2)
    {
        drawX -= getStringWidth(str);
    }

    if (alignment == 4)
    {
        drawY -= ((s32)getStringHeight(str) + 1) >> 1;
    }
    else if (alignment == 8)
    {
        drawY -= getStringHeight(str);
    }

    return font_rasterizeString(charbaseTileIndex, drawX, drawY, str);
}

static inline s32 readGlyphSample(const u8 *glyphCursor, s32 glyphBitShift)
{
    return (*glyphCursor >> glyphBitShift) & 3;
}

static inline void write4bppPixel(u8 *pixelDestination, s32 x, s32 sample, s32 colorIndex)
{
    if ((x & 1) == 0)
    {
        *pixelDestination = (*pixelDestination & 0xf0) | (sample + colorIndex);
    }
    else
    {
        *pixelDestination = (*pixelDestination & 0xf) | ((sample + colorIndex) << 4);
    }
}

static inline s32 getGlyphIndex(const FontDesc *fontDesc, u8 character)
{
    s32 index = character - fontDesc->firstChar;

    if (index < 0 || index > fontDesc->lastChar - fontDesc->firstChar)
    {
        index = 1;
    }
    return index;
}

static inline void prepareTileColumn(u32 *tiles, s32 tileRows, u16 *screenEntry, u8 *charbase)
{
    if (g_FontRenderCtx.backgroundColor == -1)
    {
        font_readCellColumn(tiles, tileRows, screenEntry, charbase);
    }
    else
    {
        font_fillTiles(tiles, tileRows, g_FontRenderCtx.backgroundColor);
    }
}

/**
 * Draw a string at pixel (x, y) into the current 4bpp or 8bpp BG render target.
 * Zero-valued glyph samples preserve the background pixels.
 * '@1'..'@4' expand once. Replacements must be nonempty and contain no placeholders.
 *
 * @return next charbase tile index
 *
 * @romaddress 0x0801d210
 */
s32 font_rasterizeString(s32 charbaseTileIndex, s32 x, s32 y, char *str)
{
    union {
        u32 words[64];
        u8 bytes[0x100];
    } tileColumn;
    char *resumeCursor;
    bool readingTemplateArg;
    u16 *screenEntry;
    u8 *charbase;
    s32 colorIndex;
    FontDesc *fontDesc;
    s32 tileRows;
    u32 *columnWords;
    s32 tileIndex = charbaseTileIndex;
    char *textCursor;

    // Prepare the first tile column and its background.
    screenEntry = font_screenEntryAt(g_FontRenderCtx.screenbase, x, y);
    charbase = g_FontRenderCtx.charbase;
    colorIndex = g_FontRenderCtx.colorIndex;
    fontDesc = g_FontRenderCtx.fontDesc;
    // Work in pixel offsets within the tile column.
    x &= TILE_WIDTH - 1;
    y &= TILE_HEIGHT - 1;
    tileRows = (y + fontDesc->height + TILE_HEIGHT - 1) >> 3;
    columnWords = tileColumn.words;
    prepareTileColumn(tileColumn.words, tileRows, screenEntry, charbase);

    textCursor = str;
    resumeCursor = textCursor;
    readingTemplateArg = FALSE;
    // Walk the text, expanding template arguments one level deep.
    while (*textCursor != '\0' || readingTemplateArg)
    {
        u8 *glyphCursor;
        s32 glyphBitShift;
        s32 glyphIndex;
        u32 glyphColumnsRemaining;

        // Resume after a template argument ends.
        if (readingTemplateArg && *textCursor == '\0')
        {
            textCursor = resumeCursor;
            readingTemplateArg = FALSE;
            if (*textCursor == '\0')
            {
                break;
            }
        }
        if (!readingTemplateArg && *textCursor == '@')
        {
            textCursor++;
            resumeCursor = textCursor;
            textCursor = g_FontTemplateArgs[FONT_TEMPLATE_ARG_INDEX(*textCursor)];
            resumeCursor++;
            readingTemplateArg = TRUE;
        }

        // BUG: An empty template argument rasterizes its NUL and advances past it.
        glyphIndex = getGlyphIndex(fontDesc, *textCursor);
        // Glyph data is a continuous stream of column-major 2bpp samples.
        glyphCursor = fontDesc->glyphBitmaps + fontDesc->glyphOffsets[glyphIndex];
        glyphBitShift = 0;
        glyphColumnsRemaining = fontDesc->glyphWidths[glyphIndex];
        while (glyphColumnsRemaining--)
        {
            u32 glyphRowsRemaining;
            s32 sample;
            u8 *pixelDestination;

            // Rasterize one glyph column in the target pixel format.
            if (g_FontRenderCtx.colors256 == TRUE)
            {
                pixelDestination = tileColumn.bytes + y * TILE_WIDTH + x;
                glyphRowsRemaining = fontDesc->height;
                while (glyphRowsRemaining--)
                {
                    if (glyphBitShift == 8)
                    {
                        glyphBitShift = 0;
                        glyphCursor++;
                    }
                    sample = readGlyphSample(glyphCursor, glyphBitShift);
                    if (sample != 0)
                    {
                        *pixelDestination = sample + colorIndex;
                    }
                    pixelDestination += TILE_WIDTH;
                    glyphBitShift += 2;
                }
            }
            else
            {
                pixelDestination = tileColumn.bytes + (y * TILE_WIDTH + x) / 2;
                glyphRowsRemaining = fontDesc->height;
                while (glyphRowsRemaining--)
                {
                    if (glyphBitShift == 8)
                    {
                        glyphBitShift = 0;
                        glyphCursor++;
                    }
                    sample = readGlyphSample(glyphCursor, glyphBitShift);
                    if (sample != 0)
                    {
                        write4bppPixel(pixelDestination, x, sample, colorIndex);
                    }
                    pixelDestination += TILE_WIDTH / 2;
                    glyphBitShift += 2;
                }
            }
            // Flush each completed 8-pixel-wide tile column.
            x++;
            if (x == TILE_WIDTH)
            {
                x = 0;
                font_writeCellColumn(columnWords, tileRows, screenEntry, charbase, tileIndex);
                tileIndex += tileRows;
                screenEntry++;
                prepareTileColumn(columnWords, tileRows, screenEntry, charbase);
            }
        }
        textCursor++;
    }

    // Flush a trailing partial tile column.
    if (x > 0)
    {
        font_writeCellColumn(columnWords, tileRows, screenEntry, charbase, tileIndex);
        tileIndex += tileRows;
    }
    g_FontRenderCtx.dirty = TRUE;
    return tileIndex;
}

// Decode a big-endian offset, ignoring its high byte.
// The high byte is zero in every ROM font asset.
#define FONT_ASSET_OFFSET(bytes) (((bytes)[1] << 16) + ((bytes)[2] << 8) + (bytes)[3])

/**
 * Decode one style's serialized font into its runtime descriptor.
 *
 * @romaddress 0x0801d478
 */
void font_loadStyleDescriptor(FontStyle styleIndex)
{
    FontAsset *asset = FontStyleInfos[styleIndex].asset;
    FontDesc *fontDesc = &g_FontDescriptors[styleIndex];

    fontDesc->firstChar = asset->d.firstChar;
    fontDesc->lastChar = asset->d.lastChar;
    fontDesc->height = asset->d.height;
    fontDesc->lineHeight = FontStyleInfos[styleIndex].lineHeight;
    fontDesc->glyphOffsets = (u16 *)(asset->bytes + FONT_ASSET_OFFSET(asset->d.glyphOffsetsOffset));
    fontDesc->glyphWidths = asset->bytes + FONT_ASSET_OFFSET(asset->d.glyphWidthsOffset);
    fontDesc->glyphBitmaps = asset->bytes + FONT_ASSET_OFFSET(asset->d.glyphBitmapsOffset);
}

/**
 * Mirror one tile horizontally at the render target's color depth.
 *
 * @romaddress 0x0801d4e0
 */
void font_flipTileHorizontal(u8 *tile)
{
    s32 rows;
    s32 pairs;
    u8 *right;
    u32 pixel;

    if (g_FontRenderCtx.colors256 == TRUE)
    {
        for (rows = 8; rows > 0; rows--)
        {
            right = tile + 7;
            for (pairs = 4; pairs > 0; pairs--)
            {
                pixel = *tile;
                *tile = *right;
                *right = pixel;
                tile++;
                right--;
            }
            tile += 4;
        }
    }
    else
    {
        for (rows = 8; rows > 0; rows--)
        {
            right = tile + 3;
            for (pairs = 2; pairs > 0; pairs--)
            {
                u8 packedPixels = *tile;
                u32 swappedPixels = (packedPixels >> 4) | (packedPixels << 4);

                *tile = (*right >> 4) | (*right << 4);
                *right = swappedPixels;
                tile++;
                right--;
            }
            tile += 2;
        }
    }
}

/**
 * Load the font descriptors and allocate the render context's tile scratch buffer.
 *
 * @romaddress 0x0801d548
 */
void font_init(void)
{
    u32 styleIndex;

    for (styleIndex = 0; styleIndex < 5; styleIndex++)
    {
        font_loadStyleDescriptor(styleIndex);
    }
    memory_memset(&g_FontRenderCtx, 0, sizeof(g_FontRenderCtx));
    g_FontRenderCtx.tileScratch = memory_malloc(0x800);
}

/**
 * Select a text background render target.
 *
 * @param bgcnt BG control register value
 *
 * @romaddress 0x0801d578
 */
void font_setBgTarget(u32 bgcnt)
{
    u16 *screenbase = (u16 *)BG_SCREEN_ADDR(BGCNT_GET_SCREENBASE(bgcnt));
    u8 *charbase = (u8 *)BG_CHAR_ADDR(BGCNT_GET_CHARBASE(bgcnt));

    g_FontRenderCtx.screenbase = screenbase;
    g_FontRenderCtx.screenbaseCopy = screenbase;
    g_FontRenderCtx.charbase = charbase;
    g_FontRenderCtx.tileStride = BGCNT_TILE_STRIDE(bgcnt);
    g_FontRenderCtx.colors256 = BGCNT_IS_8BPP(bgcnt);
}

/**
 * Select explicit screen and character bases for a 4bpp or 8bpp render target.
 *
 * @param colors256 TRUE for 8bpp, FALSE for 4bpp
 *
 * @romaddress 0x0801d5a8
 */
void font_setRenderTarget(u16 *screenbase, u8 *charbase, bool32 colors256)
{
    g_FontRenderCtx.screenbase = screenbase;
    g_FontRenderCtx.screenbaseCopy = screenbase;
    g_FontRenderCtx.charbase = charbase;
    g_FontRenderCtx.tileStride = (colors256 + 1) * 32;
    g_FontRenderCtx.colors256 = colors256;
}

/**
 * Select the font, text color, and background fill for subsequent drawing.
 *
 * @param color palette-index bias added to nonzero glyph samples. In 4bpp mode, the low
 *              nibble is the bias and the high nibble selects the palette bank.
 * @param backgroundColor fill color, or -1 to preserve existing tile pixels and palettes
 *
 * @romaddress 0x0801d5c0
 */
void font_setStyle(FontStyle styleIndex, s32 color, s32 backgroundColor)
{
    s32 paletteBits;
    g_FontRenderCtx.fontDesc = &g_FontDescriptors[styleIndex];
    g_FontRenderCtx.lineHeight = g_FontDescriptors[styleIndex].lineHeight;
    if (g_FontRenderCtx.colors256 == TRUE)
    {
        g_FontRenderCtx.colorIndex = color;
        paletteBits = 0;
    }
    else
    {
        g_FontRenderCtx.colorIndex = color & 0xf;
        paletteBits = (color >> 4) << 12;
    }
    g_FontRenderCtx.paletteBits = paletteBits;
    g_FontRenderCtx.backgroundColor = backgroundColor;
}

/**
 * Override the line advance used by the multi-line draw helpers.
 *
 * @romaddress 0x0801d5fc
 */
void font_setLineHeight(s32 lineHeight)
{
    g_FontRenderCtx.lineHeight = lineHeight;
}

/**
 * Measure str's rendered width in pixels with the current font (g_FontRenderCtx.fontDesc).
 * '@1'..'@4' insert the matching g_FontTemplateArgs entry's width ('@1' = argument 0).
 * Nested placeholders are measured recursively, unlike rendering.
 *
 * @return width in pixels
 *
 * @romaddress 0x0801d608
 */
u32 font_getStringWidth(char *str)
{
    return getStringWidth(str);
}

/**
 * Height in pixels of a single line of str in the current font (0 for an empty string).
 *
 * @romaddress 0x0801d64c
 */
u32 font_getStringHeight(char *str)
{
    return getStringHeight(str);
}

/**
 * Draw *pstr as word-wrapped lines inside a maxWidth x maxHeight box, one line advance
 * (g_FontRenderCtx.lineHeight) apart, stopping when the text or the height runs out.
 * *pstr is left at the first undrawn text.
 * Each selected line must fit font_drawLine's 63-byte source limit.
 *
 * @return next charbase tile index
 *
 * @romaddress 0x0801d668
 */
s32 font_drawLinesInBox(s32 charbaseTileIndex, s32 x, s32 y, s32 maxWidth, s32 maxHeight,
                        char **pstr, s32 alignment)
{
    u16 lineWidth;

    while (**pstr != '\0' && maxHeight >= g_FontRenderCtx.lineHeight)
    {
        charbaseTileIndex =
            font_drawLine(charbaseTileIndex, x, y, maxWidth, pstr, alignment, &lineWidth);
        y += g_FontRenderCtx.lineHeight;
        maxHeight -= g_FontRenderCtx.lineHeight;
    }
    return charbaseTileIndex;
}

/**
 * Draw all of str as word-wrapped lines of at most maxWidth px.
 * Each selected line must fit font_drawLine's 63-byte source limit.
 *
 * @return next charbase tile index
 *
 * @romaddress 0x0801d6d0
 */
s32 font_drawLines(s32 charbaseTileIndex, s32 x, s32 y, s32 maxWidth, char *str, s32 alignment)
{
    u16 lineWidth;

    while (*str != '\0')
    {
        charbaseTileIndex =
            font_drawLine(charbaseTileIndex, x, y, maxWidth, &str, alignment, &lineWidth);
        y += g_FontRenderCtx.lineHeight;
    }
    return charbaseTileIndex;
}

/**
 * Fill screen entries colStart..colEnd of one text-BG row with entry.
 *
 * @romaddress 0x0801d714
 */
void font_fillScreenRow(s32 colStart, s32 row, s32 colEnd, u16 entry)
{
    volatile u16 fill;

    if (colStart <= colEnd)
    {
        u16 *dst = g_FontRenderCtx.screenbase + (row * 32 + colStart);
        s32 bytes = (colEnd - colStart + 1) * 2;

        fill = entry;
        DMA3_FILL16(&fill, dst, bytes >> 1);
    }
}

/**
 * Clear screen entries colStart..colEnd of one text-BG row to tile 0.
 *
 * @romaddress 0x0801d75c
 */
void font_clearScreenRow(s32 colStart, s32 row, s32 colEnd)
{
    volatile u16 zero;

    if (colStart <= colEnd)
    {
        u16 *dst = g_FontRenderCtx.screenbase + (row * 32 + colStart);
        s32 bytes = (colEnd - colStart + 1) * 2;

        zero = 0;
        DMA3_FILL16(&zero, dst, bytes >> 1);
    }
}

static inline void setTemplateArgInline(char *src, s32 argIndex)
{
    char *dst = g_FontTemplateArgs[argIndex];

    while (*src != '\0')
    {
        *dst++ = *src++;
    }
    *dst = '\0';
}

/**
 * Set a template argument by copying src (NUL-terminated).
 *
 * @param argIndex zero-based argument index ('@1' = argument 0)
 *
 * @romaddress 0x0801d7a4
 */
void font_setTemplateArg(char *src, s32 argIndex)
{
    setTemplateArgInline(src, argIndex);
}

/**
 * Set template argument 0 ('@1') by copying src (NUL-terminated).
 *
 * @romaddress 0x0801d7c8
 */
void font_setTemplateArg0(char *src)
{
    setTemplateArgInline(src, 0);
}

/**
 * Set template argument 1 ('@2') by copying src (NUL-terminated).
 *
 * @romaddress 0x0801d7e8
 */
void font_setTemplateArg1(char *src)
{
    setTemplateArgInline(src, 1);
}

/**
 * Set template argument 2 ('@3') by copying src (NUL-terminated).
 *
 * @romaddress 0x0801d80c
 */
void font_setTemplateArg2(char *src)
{
    setTemplateArgInline(src, 2);
}

static inline void setIntTemplateArgInline(s32 value, s32 argIndex)
{
    text_formatInt(value, g_FontTemplateArgs[argIndex]);
}

/**
 * Set a template argument to value formatted as decimal with separators.
 *
 * @param argIndex zero-based argument index ('@1' = argument 0)
 *
 * @romaddress 0x0801d830
 */
void font_setIntTemplateArg(s32 value, s32 argIndex)
{
    setIntTemplateArgInline(value, argIndex);
}

/**
 * Set template argument 0 ('@1') to value formatted as decimal with separators.
 *
 * @romaddress 0x0801d844
 */
void font_setIntTemplateArg0(s32 value)
{
    setIntTemplateArgInline(value, 0);
}

/**
 * Set template argument 1 ('@2') to value formatted as decimal with separators.
 *
 * @romaddress 0x0801d854
 */
void font_setIntTemplateArg1(s32 value)
{
    setIntTemplateArgInline(value, 1);
}

/**
 * Set template argument 2 ('@3') to value formatted as decimal with separators.
 *
 * @romaddress 0x0801d868
 */
void font_setIntTemplateArg2(s32 value)
{
    setIntTemplateArgInline(value, 2);
}

/**
 * Fill tileCount tiles with one color, at the target BG's depth (color is a palette index
 * for 8bpp, a nibble for 4bpp).
 *
 * @romaddress 0x0801d87c
 */
void font_fillTiles(u32 *tiles, u32 tileCount, u32 color)
{
    u32 words;

    if (g_FontRenderCtx.colors256 == TRUE)
    {
        color += (color << 24) + (color << 16) + (color << 8);
        words = tileCount << 4;
    }
    else
    {
        color &= 0xf;
        color += color << 4;
        color += color << 8;
        color += color << 16;
        words = tileCount << 3;
    }
    while (words--)
    {
        *tiles++ = color;
    }
}

/**
 * Gather the tiles of a column of rows screen cells (one screen entry per BG row, starting
 * at screenEntry) from charbase into dst, undoing a horizontal flip flag on the way.
 *
 * @romaddress 0x0801d8cc
 */
void font_readCellColumn(u32 *dst, u32 rows, const u16 *screenEntry, const u8 *charbase)
{
    while (rows--)
    {
        const u32 *src =
            (const u32 *)(charbase + (*screenEntry & 0x3ff) * g_FontRenderCtx.tileStride);
        u32 words = (u32)g_FontRenderCtx.tileStride >> 2;

        while (words--)
        {
            *dst++ = *src++;
        }
        if (*screenEntry & 0x400)
        {
            font_flipTileHorizontal((u8 *)dst - g_FontRenderCtx.tileStride);
        }
        screenEntry += 32;
    }
}

/**
 * Write rows tiles from src into charbase at tileIndex.. and point a column of screen
 * cells (one per BG row, starting at screenEntry) at them. A 4bpp cell keeps its palette
 * bits when g_FontRenderCtx.backgroundColor is -1, else takes g_FontRenderCtx.paletteBits.
 *
 * @romaddress 0x0801d958
 */
void font_writeCellColumn(const u32 *src, u32 rows, u16 *screenEntry, u8 *charbase, s32 tileIndex)
{
    while (rows--)
    {
        u32 *dst = (u32 *)(charbase + tileIndex * g_FontRenderCtx.tileStride);
        u32 words = (u32)g_FontRenderCtx.tileStride >> 2;

        while (words--)
        {
            *dst++ = *src++;
        }
        if (g_FontRenderCtx.colors256 == TRUE)
        {
            *screenEntry = tileIndex;
        }
        else if (g_FontRenderCtx.backgroundColor == -1)
        {
            *screenEntry = (*screenEntry & 0xf000) | tileIndex;
        }
        else
        {
            *screenEntry = g_FontRenderCtx.paletteBits | tileIndex;
        }
        screenEntry += 32;
        tileIndex++;
    }
}

/**
 * Screen entry of the 32-cell-wide text BG covering pixel (x, y).
 *
 * @romaddress 0x0801d9f8
 */
u16 *font_screenEntryAt(u16 *screenbase, s32 x, s32 y)
{
    y /= 8;
    x /= 8;
    return screenbase + (y * 32 + x);
}

/**
 * Width of ch's glyph in fontDesc (chars outside firstChar..lastChar get the fallback
 * width, a NUL 0).
 *
 * @romaddress 0x0801da1c
 */
s32 font_getGlyphWidth(FontDesc *fontDesc, s32 ch)
{
    u8 c = ch;
    s32 width = 0;

    if (c != '\0')
    {
        s32 index = c - fontDesc->firstChar;

        if (index >= 0 && index <= fontDesc->lastChar - fontDesc->firstChar)
        {
            width = fontDesc->glyphWidths[index];
        }
        else
        {
            width = fontDesc->glyphWidths[1];
        }
    }
    return width;
}

/**
 * Set template argument 3 ('@4') by copying src (NUL-terminated).
 *
 * @romaddress 0x0801da4c
 */
void font_setTemplateArg3(char *src)
{
    setTemplateArgInline(src, 3);
}
