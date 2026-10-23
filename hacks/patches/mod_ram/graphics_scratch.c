#include "patches/mod_ram/graphics_scratch.h"
#include "font.h"
#include "memory.h"
#include "text.h"
#include "variables.h"
#include <stdint.h>

#define TEXT_MAP_OFFSET 0x780
#define TEXT_MAP_SIZE 0x74
#define TEXT_MAP_BORROWED 1
#define TEXT_TILES_BORROWED 2

static u8 borrowed;
static u32 uses;

_Static_assert(TEXT_MAP_OFFSET + TEXT_MAP_SIZE <= 0x800, "font scratch overflow");

static bool isTextCaller(const void *caller, u32 offset)
{
    return ((uintptr_t)caller & ~(uintptr_t)1) ==
           (((uintptr_t)text_objRasterizeString & ~(uintptr_t)1) + offset);
}

bool modRam_borrowTextMap(const void *caller, void **out)
{
    if (isTextCaller(caller, 0x1c) && borrowed == 0 && g_FontRenderCtx.tileScratch != NULL)
    {
        *out = (u8 *)g_FontRenderCtx.tileScratch + TEXT_MAP_OFFSET;
        borrowed = TEXT_MAP_BORROWED;
        uses++;
        return TRUE;
    }
    return FALSE;
}

bool modRam_borrowTextTiles(const void *caller, size_t size, void **out)
{
    if (isTextCaller(caller, 0x38) && borrowed == TEXT_MAP_BORROWED && size <= TEXT_MAP_OFFSET)
    {
        *out = g_FontRenderCtx.tileScratch;
        borrowed |= TEXT_TILES_BORROWED;
        memory_memset(*out, 0, size);
        return TRUE;
    }
    return FALSE;
}

bool modRam_releaseTextScratch(void *pointer)
{
    if ((borrowed & TEXT_MAP_BORROWED) &&
        pointer == (u8 *)g_FontRenderCtx.tileScratch + TEXT_MAP_OFFSET)
    {
        borrowed &= ~TEXT_MAP_BORROWED;
        return TRUE;
    }
    if ((borrowed & TEXT_TILES_BORROWED) && pointer == g_FontRenderCtx.tileScratch)
    {
        borrowed &= ~TEXT_TILES_BORROWED;
        return TRUE;
    }
    return FALSE;
}

u32 modRam_sharedTextUses(void)
{
    return uses;
}
