#include "patches/experimental/experimental.h"

#include "gba_io.h"
#include "lib/fx_kit.h"
#include "patches/mod_save/save_sram.h"
#include "variables.h"

// ARM instruction words of the native audio mixer, which runs from IWRAM.
#define ARM_BRANCH_OVER_BLEND 0xEA000004u // b +6 words
#define ARM_LOAD_NEXT_SAMPLE 0xE7DEA847u // ldrb sl, [lr, r7, asr #16]
#define ARM_CMP_VOICE_INDEX 0xE3510000u // cmp r1, #imm
#define ARM_BEQ_SKIP_VOICE 0x0A000091u // beq to the zero-volume voice path
#define ARM_B_SKIP_VOICE 0xEA000091u
#define VOICE_POOL_SIZE 32

// Each site starts a six instruction blend of two neighboring samples. Branching over it
// leaves the first sample, which is the nearest-neighbor result.
static const u32 BlendSites[] = {
    0x030007e0, 0x03000800, 0x0300082c, 0x0300084c, 0x030008e8, 0x03000908,
    0x03000934, 0x03000954, 0x030009d8, 0x030009f8, 0x03000a1c, 0x03000a3c,
};

// The two compares that bound the free voice search when a sound effect starts.
static const u32 VoiceSearchSites[] = {0x0300010c, 0x0300012c};

// Branches a silent voice to the path that only advances its position.
#define MuteSite 0x03000cc8

// Undocumented memory control register. Bits 24-27 hold 15 minus the work RAM wait states.
#define REG_MEMCTRL (*(volatile u32 *)0x04000800)
#define MEMCTRL_EWRAM_WAIT_MASK 0x0f000000u
#define MEMCTRL_EWRAM_WAIT_2 0x0d000000u
#define MEMCTRL_EWRAM_WAIT_1 0x0e000000u

// Live effect limits, the first entry for none.
static const u8 FxCaps[] = {0, 24, 16, 8};

static const u8 VoiceCaps[] = {VOICE_POOL_SIZE, 16, 12, 8, 6};
#define VOICE_CAP_COUNT (s32)(sizeof(VoiceCaps) / sizeof(VoiceCaps[0]))

// Written into the state of a blank save, so a test ROM can start with a toggle on.
const volatile struct ExperimentalState ExperimentalDefaultState = {.tag = EXPERIMENTAL_TAG};

struct ExperimentalState g_ExperimentalState;

static u32 *mixerWord(u32 address)
{
    return (u32 *)(address);
}

static u32 nativeWord(u32 address)
{
    return ((const u32 *)IwramStaticsImage)[(address - (u32)IWRAM_BASE) / sizeof(u32)];
}

/** TRUE when the IWRAM image still has the mixer this patch was written against. */
static bool32 isKnownMixer(void)
{
    s32 i;

    for (i = 0; i < (s32)(sizeof(BlendSites) / sizeof(BlendSites[0])); i++)
    {
        if (nativeWord(BlendSites[i]) != ARM_LOAD_NEXT_SAMPLE)
        {
            return FALSE;
        }
    }
    for (i = 0; i < (s32)(sizeof(VoiceSearchSites) / sizeof(VoiceSearchSites[0])); i++)
    {
        if (nativeWord(VoiceSearchSites[i]) != (ARM_CMP_VOICE_INDEX | (VOICE_POOL_SIZE - 1)))
        {
            return FALSE;
        }
    }
    return nativeWord(MuteSite) == ARM_BEQ_SKIP_VOICE;
}

/** Rewrite the mixer's IWRAM code from the current state, with interrupts masked. */
static void applyAudio(void)
{
    u16 ime;
    s32 i;
    u32 voiceCompare = nativeWord(VoiceSearchSites[0]);

    if (!isKnownMixer())
    {
        return;
    }
    if (g_ExperimentalState.voiceCap < VOICE_CAP_COUNT)
    {
        voiceCompare = ARM_CMP_VOICE_INDEX | (VoiceCaps[g_ExperimentalState.voiceCap] - 1);
    }
    ime = REG_IME;
    REG_IME = 0;
    for (i = 0; i < (s32)(sizeof(BlendSites) / sizeof(BlendSites[0])); i++)
    {
        *mixerWord(BlendSites[i]) =
            g_ExperimentalState.fastMixer ? ARM_BRANCH_OVER_BLEND : nativeWord(BlendSites[i]);
    }
    for (i = 0; i < (s32)(sizeof(VoiceSearchSites) / sizeof(VoiceSearchSites[0])); i++)
    {
        *mixerWord(VoiceSearchSites[i]) = voiceCompare;
    }
    *mixerWord(MuteSite) = g_ExperimentalState.mute ? ARM_B_SKIP_VOICE : nativeWord(MuteSite);
    REG_IME = ime;
}

static void applyMemory(void)
{
    REG_MEMCTRL = (REG_MEMCTRL & ~MEMCTRL_EWRAM_WAIT_MASK) |
                  (g_ExperimentalState.fastRam ? MEMCTRL_EWRAM_WAIT_1 : MEMCTRL_EWRAM_WAIT_2);
}

static void applyState(void)
{
    applyAudio();
    applyMemory();
    hackLib_fxCap = FxCaps[g_ExperimentalState.fxCap];
}

void experimental_loadState(void)
{
    GLOBAL_MOD_OPTIONS_READ(experimentalState, &g_ExperimentalState);
    if (g_ExperimentalState.tag != EXPERIMENTAL_TAG)
    {
        g_ExperimentalState = ExperimentalDefaultState;
    }
    if (g_ExperimentalState.tag2 != EXPERIMENTAL_TAG)
    {
        g_ExperimentalState.framePacing = ExperimentalDefaultState.framePacing;
        g_ExperimentalState.fastRam = ExperimentalDefaultState.fastRam;
        g_ExperimentalState.fxCap = ExperimentalDefaultState.fxCap;
        g_ExperimentalState.unused = 0;
        g_ExperimentalState.tag2 = EXPERIMENTAL_TAG;
    }
    if (g_ExperimentalState.voiceCap >= VOICE_CAP_COUNT)
    {
        g_ExperimentalState.voiceCap = 0;
    }
    if (g_ExperimentalState.cameraEase > 2)
    {
        g_ExperimentalState.cameraEase = 0;
    }
    applyState();
}

void experimental_useDefaults(void)
{
    g_ExperimentalState = ExperimentalDefaultState;
    applyState();
}

void experimental_applyState(void)
{
    applyState();
}

#if HACK_mod_options
static const char *const ExperimentalOffOnNames[2] = {"Off", "On"};
static const char *const ExperimentalVoiceCapNames[5] = {"Off", "16", "12", "8", "6"};
static const char *const ExperimentalCameraEaseNames[3] = {"Off", "Light", "Strong"};
static const char *const ExperimentalFxCapNames[4] = {"Off", "24", "16", "8"};

static void commitState(void)
{
    GLOBAL_MOD_OPTIONS_WRITE(experimentalState, &g_ExperimentalState);
    applyState();
}

static u8 get_fastMixer(void)
{
    return g_ExperimentalState.fastMixer;
}

static void set_fastMixer(u8 value)
{
    g_ExperimentalState.fastMixer = value;
    commitState();
}

static u8 get_voiceCap(void)
{
    return g_ExperimentalState.voiceCap;
}

static void set_voiceCap(u8 value)
{
    g_ExperimentalState.voiceCap = value;
    commitState();
}

static u8 get_mute(void)
{
    return g_ExperimentalState.mute;
}

static void set_mute(u8 value)
{
    g_ExperimentalState.mute = value;
    commitState();
}

static u8 get_cameraEase(void)
{
    return g_ExperimentalState.cameraEase;
}

static void set_cameraEase(u8 value)
{
    g_ExperimentalState.cameraEase = value;
    commitState();
}

static u8 get_fxCap(void)
{
    return g_ExperimentalState.fxCap;
}

static void set_fxCap(u8 value)
{
    g_ExperimentalState.fxCap = value;
    commitState();
}

static u8 get_fastRam(void)
{
    return g_ExperimentalState.fastRam;
}

static void set_fastRam(u8 value)
{
    g_ExperimentalState.fastRam = value;
    commitState();
}

static u8 get_framePacing(void)
{
    return g_ExperimentalState.framePacing;
}

static void set_framePacing(u8 value)
{
    g_ExperimentalState.framePacing = value;
    commitState();
}

const struct ModOptionEntry ExperimentalOptions[EXPERIMENTAL_OPTION_COUNT] = {
    {"Fast Mixer", ExperimentalOffOnNames, 2, 0, get_fastMixer, set_fastMixer,
     "Noticeably less lag.\nRougher sound."},
    {"Voice Cap", ExperimentalVoiceCapNames, 5, 0, get_voiceCap, set_voiceCap,
     "Limits sound voices.\nDrops sounds, less lag."},
    {"Mute Audio", ExperimentalOffOnNames, 2, 0, get_mute, set_mute,
     "Silences all audio.\nRemoves the most lag."},
    {"Camera Ease", ExperimentalCameraEaseNames, 3, 0, get_cameraEase, set_cameraEase,
     "Test: camera glides after\nthe player."},
    {"Frame Pacing", ExperimentalOffOnNames, 2, 0, get_framePacing, set_framePacing,
     "Test: less idle time when\nbehind. Less stutter."},
    {"Fast RAM", ExperimentalOffOnNames, 2, 0, get_fastRam, set_fastRam,
     "Faster work RAM. GBA/SP\nonly, may crash others."},
};

const struct ModOptionEntry ExperimentalOptions2[EXPERIMENTAL_OPTION2_COUNT] = {
    {"Effect Cap", ExperimentalFxCapNames, 4, 0, get_fxCap, set_fxCap,
     "Limits new skill effects.\nLess lag, fewer visuals."},
};
#endif
