#include "lib/combat_event.h"

static u8 dispatchDepth;

HackLibCombatEvent hackLib_captureCombatEvent(Actor *attacker, Actor *victim,
                                              HackLibDamageResult result, bool melee)
{
    const HackLibDamageSource *source = hackLib_currentDamageSource();
    HackLibCombatEvent event = {attacker,
                                victim,
                                {attacker,
                                 melee ? HACKLIB_DAMAGE_WEAPON : HACKLIB_DAMAGE_PROJECTILE,
                                 HACKLIB_ABILITY_NONE, 0, HACKLIB_ELEMENT_NONE, FALSE},
                                result,
                                melee,
                                FALSE,
                                FALSE,
                                HACKLIB_ELEMENT_NONE,
                                FALSE};
    if (source != NULL)
    {
        event.source = *source;
    }
    return event;
}

bool hackLib_dispatchCombatEvent(const HackLibCombatEvent *event,
                                 const HackLibCombatHandlers *handlers, void *context)
{
    if (event == NULL || handlers == NULL || dispatchDepth >= HACKLIB_MAX_DAMAGE_DEPTH ||
        event->source.depth >= HACKLIB_MAX_DAMAGE_DEPTH)
    {
        return FALSE;
    }
    // A local snapshot also protects callbacks from changes to the caller's event storage.
    const HackLibCombatEvent snapshot = *event;
    const HackLibDamageSource *previous = hackLib_swapDamageSource(&snapshot.source);
    dispatchDepth++;
    if (snapshot.result.hit)
    {
        if (snapshot.result.killed && handlers->kill != NULL)
        {
            handlers->kill(&snapshot, context);
        }
        if (handlers->hit != NULL)
        {
            handlers->hit(&snapshot, context);
        }
    }
    if (handlers->resolved != NULL)
    {
        handlers->resolved(&snapshot, context);
    }
    dispatchDepth--;
    hackLib_swapDamageSource(previous);
    return TRUE;
}
