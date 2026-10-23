#include "text.h"
#include "variables.h"
#if HACK_ringwraith && HACK_skill_expansion
#include "patches/ringwraith/wraith_gear.h"
#endif

#define TEXT_OVERRIDE_KEY(language, textId) (((u32)(language) << 16) | (u32)(textId))

#define TEXT_OVERRIDE(language, textId, replacement)                                               \
    _Static_assert((u32)(language) < LANGUAGE_COUNT, "invalid text override language");            \
    _Static_assert((u32)(textId) <= 0xffff, "invalid text override ID");                           \
    _Static_assert(sizeof("" replacement) <= TEXT_STRING_BUFFER_SIZE, "text override too long");
#include "text_overrides.inc"
#undef TEXT_OVERRIDE

static const u8 *findString(s32 textId)
{
    if ((u32)textId > 0xffff)
    {
        return NULL;
    }
#if HACK_ringwraith && HACK_skill_expansion
    if (wraithGear_findString(textId) != NULL)
    {
        return (const u8 *)wraithGear_findString(textId);
    }
#endif
    switch (TEXT_OVERRIDE_KEY(text_getLanguage(), textId))
    {
#define TEXT_OVERRIDE(language, textId, replacement)                                               \
    case TEXT_OVERRIDE_KEY(language, textId):                                                      \
        return (const u8 *)("" replacement);
#include "text_overrides.inc"
#undef TEXT_OVERRIDE
    default:
        return NULL;
    }
}

TextDecodeResult textOverrides_decodeHuffmanString(s32 id, u8 *dst, s32 maxLen)
{
    const u8 *stream;
    s32 bitIndex;
    u8 bits;
    s32 length;
    u16 node;

    if (g_TextPackage == NULL)
    {
        return TEXT_DECODE_NO_PACKAGE;
    }

    // CHANGE START: Check if we should return an override
    const u8 *replacement;
    replacement = findString(id);
    if (replacement != NULL)
    {
        for (length = 0; length < maxLen; length++)
        {
            dst[length] = replacement[length];
            if (dst[length] == '\0')
            {
                return TEXT_DECODE_OK;
            }
        }
        return TEXT_DECODE_BUFFER_FULL;
    }
    // CHANGE END

    // Original Huffman decoder.
    stream = g_TextPackage->bytes + g_TextStringOffsets[id];
    length = 0;
    bitIndex = 0;
    bits = *stream++;

    do
    {
        node = HUFFMAN_ROOT_NODE;
        while (node >= HUFFMAN_ROOT_NODE)
        {
            if ((bits >> bitIndex) & 1)
            {
                node = g_TextHuffmanTree[node - HUFFMAN_ROOT_NODE].right;
            }
            else
            {
                node = g_TextHuffmanTree[node - HUFFMAN_ROOT_NODE].left;
            }

            bitIndex++;
            if (bitIndex == 8)
            {
                bitIndex = 0;
                bits = *stream++;
            }
        }

        dst[length++] = node;
        if (dst[length - 1] == 0)
        {
            return TEXT_DECODE_OK;
        }
    } while (length != maxLen);

    return TEXT_DECODE_BUFFER_FULL;
}
