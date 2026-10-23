#include "rng.h"
#include "save.h"
#include "sfx.h"
#include "variables.h"

/**
 * Play a sound effect. Returns the handle of the playing instance, or 0 if none played.
 *
 * @romaddress 0x08046eec
 */
u32 sfx_play(s32 sfxId)
{
    u32 handle = 0;

    if (g_SaveGlobals.d.optionsFlags.p & OPTION_FLAG_SFX_ENABLED)
    {
        const SfxEntry *entry = (const SfxEntry *)((u8 *)SfxEntries + sfxId * 4);

        switch (entry->kind)
        {
        case SFX_ENTRY_SINGLE:
            handle = sfx_driverPlay(SoundData[SfxSingles[entry->index].sound], 1, 0);
            sfx_driverUnknown0804dc14(handle, SfxSingles[entry->index].volume);
            break;
        case SFX_ENTRY_VARIANT:
        {
            u8 variant = rng_rollUpTo(7);
            const SfxVariant *sound = &SfxVariantSets[entry->index][variant];

            if (sound->sound != 0xff)
            {
                handle = sfx_driverPlay(SoundData[sound->sound], 1, 0);
                sfx_driverUnknown0804dc14(handle, SfxVariantSets[entry->index][variant].volume);
            }
            break;
        }
        }
        return handle;
    }
    return 0;
}

/**
 * As sfx_play, but returns handle without playing anything while that instance is still playing.
 *
 * @romaddress 0x08046fa0
 */
u32 sfx_playUnlessPlaying(s32 sfxId, u32 handle)
{
    u32 newHandle = 0;

    if (g_SaveGlobals.d.optionsFlags.p & OPTION_FLAG_SFX_ENABLED)
    {
        if (!sfx_isPlaying(handle))
        {
            const SfxEntry *entry = (const SfxEntry *)((u8 *)SfxEntries + sfxId * 4);

            switch (entry->kind)
            {
            case SFX_ENTRY_SINGLE:
                newHandle = sfx_driverPlay(SoundData[SfxSingles[entry->index].sound], 1, 0);
                sfx_driverUnknown0804dc14(newHandle, SfxSingles[entry->index].volume);
                break;
            case SFX_ENTRY_VARIANT:
            {
                u8 variant = rng_rollUpTo(7);
                const SfxVariant *sound = &SfxVariantSets[entry->index][variant];

                if (sound->sound != 0xff)
                {
                    newHandle = sfx_driverPlay(SoundData[sound->sound], 1, 0);
                    sfx_driverUnknown0804dc14(newHandle,
                                              SfxVariantSets[entry->index][variant].volume);
                }
                break;
            }
            }
            return newHandle;
        }
        return handle;
    }
    return 0;
}
