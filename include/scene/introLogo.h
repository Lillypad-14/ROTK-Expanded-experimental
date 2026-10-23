#pragma once

#include "types.h"

typedef struct IntroLogoFrame
{
    u32 field_0x0;
    const void *bg0Asset;
    const void *bg1Asset;
    u32 field_0xc;
} IntroLogoFrame;

// Scene 0x03: intro logo.
void scene_introLogo_enter(void);
void scene_introLogo_tick(void);
void scene_introLogo_exit(void);
