#pragma once

#include "sprite.h"

void fastSprite_composeFrame(const SpriteFrameSetHeader *frames, u16 frameIndex, u8 flags,
                             const Vector2Int *position, u16 tileNumber, const OamEntryWords *base,
                             Actor *actor);
