#pragma once
#include "patches/mod_options/mod_options.h"
void deliveryPreview_playerTick(u8 playerIndex);
void deliveryPreview_drawTelemetry(u8 playerIndex);
#define DELIVERY_PREVIEW_OPTION_COUNT 7
extern const struct ModOptionEntry DeliveryPreviewOptions[DELIVERY_PREVIEW_OPTION_COUNT];
