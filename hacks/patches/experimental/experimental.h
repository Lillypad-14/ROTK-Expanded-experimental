#pragma once

// Experimental: a mod_options page of off-by-default performance and engine experiments.
// Every toggle leaves the vanilla behavior untouched until it is switched on.

#include "types.h"
#include "vector2.h"

/**
 * State bytes: bit 7 of each byte is a tag, 0 once written and 1 on a blank byte (the 0xFF
 * migration fill), so an unwritten byte reads as the factory defaults.
 */
struct ExperimentalState
{
    // Skip the sample blending step in the audio mixer.
    u8 fastMixer : 1;
    // Index into the voice cap table, 0 for no cap.
    u8 voiceCap : 3;
    // Skip every voice in the audio mixer.
    u8 mute : 1;
    // Camera easing strength, 0 for a camera locked to the player.
    u8 cameraEase : 2;
    u8 tag : 1;
    // Run the next game tick while the previous frame waits for its vblank.
    u8 framePacing : 1;
    // Run work RAM with one wait state instead of two.
    u8 fastRam : 1;
    // Index into the effect cap table, 0 for no cap.
    u8 fxCap : 2;
    u8 unused : 3;
    u8 tag2 : 1;
};

#define EXPERIMENTAL_TAG 0 // written bytes read tag 0; the 0xFF blank fill reads tag 1

extern struct ExperimentalState g_ExperimentalState;

/** Boot task (runTasks): load the persisted state byte and apply it to the audio mixer. */
void experimental_loadState(void);

/** Test arena boot: start from the factory defaults without reading the save. */
void experimental_useDefaults(void);

/** Apply g_ExperimentalState after a co-op client replaced it with the host's. */
void experimental_applyState(void);

/** Replaces the player position getter that gfx_updatePlayerCameraScroll reads. */
void cameraEase_getPlayerPosition(Vector2Fp16 *out, u8 playerIndex);

/** Replaces the rotk_main game loop. */
void framePacing_mainLoop(void);

/** Called before a frame's sprites are written: waits for the previous frame's commit. */
void framePacing_awaitCommit(void);

// The patch's mod_options page entries (compiled when HACK_mod_options).
#include "patches/mod_options/mod_options.h"

#define EXPERIMENTAL_OPTION_COUNT 6
extern const struct ModOptionEntry ExperimentalOptions[EXPERIMENTAL_OPTION_COUNT];

#define EXPERIMENTAL_OPTION2_COUNT 1
extern const struct ModOptionEntry ExperimentalOptions2[EXPERIMENTAL_OPTION2_COUNT];
