#include "text.h"
#include "variables.h"

/**
 * Select the compressed text package used for subsequent string decoding.
 *
 * @return TEXT_DECODE_OK
 *
 * @romaddress 0x0802997c
 */
TextDecodeResult text_setLanguagePackage(const TextPackage *package)
{
    s32 stringOffsetsOffset;

    g_TextPackage = package;

    stringOffsetsOffset = package->d.stringOffsetsOffset;
    g_TextHuffmanTree = package->d.tree;
    g_TextStringOffsets = (const s32 *)&package->bytes[stringOffsetsOffset];

    return TEXT_DECODE_OK;
}

/**
 * Decode a localized string into dst, including its NUL terminator.
 * TEXT_DECODE_BUFFER_FULL leaves dst unterminated.
 *
 * @param maxLen Positive output capacity, including the terminator.
 * @return A TextDecodeResult.
 *
 * @romaddress 0x080299a0
 */
TextDecodeResult text_decodeHuffmanString(s32 id, u8 *dst, s32 maxLen)
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
