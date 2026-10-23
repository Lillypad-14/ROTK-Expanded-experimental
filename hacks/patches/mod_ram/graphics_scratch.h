#pragma once

#include "types.h"

bool modRam_borrowTextMap(const void *caller, void **out);
bool modRam_borrowTextTiles(const void *caller, size_t size, void **out);
bool modRam_releaseTextScratch(void *pointer);
u32 modRam_sharedTextUses(void);
