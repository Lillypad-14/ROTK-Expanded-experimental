#pragma once

#include "types.h"

/** Decompress a tagged chunk and return its uncompressed byte count. */
u32 decompress_chunk(const void *source, void *destination);
