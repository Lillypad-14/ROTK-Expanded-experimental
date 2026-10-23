#include "lib/combat_source.h"

static const HackLibDamageSource *currentSource;

const HackLibDamageSource *hackLib_swapDamageSource(const HackLibDamageSource *source)
{
    const HackLibDamageSource *previous = currentSource;

    currentSource = source;
    return previous;
}

const HackLibDamageSource *hackLib_currentDamageSource(void)
{
    return currentSource;
}
