#include "gba_io.h"
#include "sprite.h"
#include "variables.h"

/**
 * Copy or decompress the actor's current tile stream to its allocated OBJ tiles.
 *
 * @romaddress 0x08049b34
 */
void sprite_uploadActorFrame(Actor *actor, const void *tiles)
{
    u32 decoded;
    u8 mode =
        sprite_getFrameCompression(actor->spriteAnimation->frames, actor->currentAnimationFrame);
    void *destination = (void *)(OBJ_TILE_RAM + (actor->tileNumber << 5));
    switch (mode)
    {
    case SPRITE_COMPRESSION_RL:
        bios_rlUnCompVram(tiles, destination);
        break;
    case SPRITE_COMPRESSION_LZ77:
        ((void (*)(const void *, void *, u32 *))g_DecompressLz77)(tiles, destination, &decoded);
        break;
    case SPRITE_COMPRESSION_RAW:
        decoded = actor->gfxSize;
        bios_cpuFastSet(tiles, destination, (decoded << 9) >> 11);
        break;
    }
}

/**
 * Copy or decompress one frame's tile stream to an OBJ tile allocation.
 *
 * @romaddress 0x08049bac
 */
void sprite_uploadFrame(const SpriteFrameSetHeader *frames, const void *tiles, u16 tileNumber,
                        u16 frameIndex, u16 tileBytes)
{
    u32 decoded;
    u8 mode = sprite_getFrameCompression(frames, frameIndex);
    void *destination = (void *)(OBJ_TILE_RAM + (tileNumber << 5));
    switch (mode)
    {
    case SPRITE_COMPRESSION_RL:
        bios_rlUnCompVram(tiles, destination);
        break;
    case SPRITE_COMPRESSION_LZ77:
        ((void (*)(const void *, void *, u32 *))g_DecompressLz77)(tiles, destination, &decoded);
        break;
    case SPRITE_COMPRESSION_RAW:
        bios_cpuFastSet(tiles, destination, tileBytes >> 2);
        break;
    }
}
