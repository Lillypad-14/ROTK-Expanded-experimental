#pragma once

#include "types.h"

// Scene 0x11: in-game dialog overlay.
void scene_inGameDialog_enter(void);
void scene_inGameDialog_exit(void);
void scene_inGameDialog_tick(void);
void scene_inGameDialog_unknown0801ac78(void);

/** DialogLineInfos[textId]: what a dialog line shows besides its text. */
typedef struct DialogLineInfo
{
    const void *speakerPicture; // 7x8-tile BG asset; NULL: the active hero's own
    // Value for font template argument 0, or a CreditsLineKinds entry for credits.
    const s16 *templateArg0Value;
} DialogLineInfo;
