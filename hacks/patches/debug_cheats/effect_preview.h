#pragma once

#include "patches/mod_options/mod_options.h"

void effectPreview_playerTick(u8 playerIndex);
#define EFFECT_PREVIEW_OPTION_COUNT 3
extern const struct ModOptionEntry EffectPreviewOptions[EFFECT_PREVIEW_OPTION_COUNT];
