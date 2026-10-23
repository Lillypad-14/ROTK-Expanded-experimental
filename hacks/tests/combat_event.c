#include "lib/combat_event.h"
#include <assert.h>
#include <stdio.h>

static Actor attacker, victim, summon;
static u32 kills, hits, resolutions, order, nesting;
static HackLibCombatEvent mutableEvent;

static void killed(const HackLibCombatEvent *event, void *context)
{
    assert(context == &order && event->result.killed && event->result.hit);
    assert(order++ == 0);
    kills++;
    // Caller storage can change while the captured event remains stable.
    mutableEvent.result = (HackLibDamageResult){0};
}

static void hit(const HackLibCombatEvent *event, void *context)
{
    assert(context == &order && event->result.damageDealt == 20);
    assert(event->criticalKnown && event->critical);
    assert(order++ == 1);
    hits++;
}

static void resolved(const HackLibCombatEvent *event, void *context)
{
    assert(context == &order);
    assert(hackLib_currentDamageSource()->origin == event->source.origin);
    if (event->result.hit)
    {
        assert(order++ == 2);
    }
    resolutions++;
}

static void nested(const HackLibCombatEvent *event, void *context)
{
    const HackLibDamageSource *parent = hackLib_currentDamageSource();
    const HackLibCombatHandlers handlers = {NULL, NULL, nested};
    (void)context;
    assert(parent->origin == event->source.origin);
    nesting++;
    HackLibCombatEvent child = *event;
    child.source.origin = &summon;
    child.source.kind = HACKLIB_DAMAGE_SECONDARY;
    // Even a callback which forgets to advance source.depth is bounded.
    bool dispatched = hackLib_dispatchCombatEvent(&child, &handlers, NULL);
    assert(dispatched == (nesting < HACKLIB_MAX_DAMAGE_DEPTH));
    assert(hackLib_currentDamageSource() == parent);
    nesting--;
    resolutions++;
}

int main(void)
{
    const HackLibCombatHandlers handlers = {killed, hit, resolved};
    HackLibDamageResult result = hackLib_damageResult(20, -5, TRUE);
    mutableEvent = hackLib_captureCombatEvent(&attacker, &victim, result, TRUE);
    assert(mutableEvent.source.kind == HACKLIB_DAMAGE_WEAPON && mutableEvent.melee);
    assert(mutableEvent.source.abilityRow == HACKLIB_ABILITY_NONE);
    assert(!mutableEvent.criticalKnown && !mutableEvent.critical);
    mutableEvent.criticalKnown = mutableEvent.critical = TRUE;
    assert(hackLib_dispatchCombatEvent(&mutableEvent, &handlers, &order));
    assert(kills == 1 && hits == 1 && resolutions == 1 && order == 3);
    assert(hackLib_currentDamageSource() == NULL);

    HackLibDamageSource source = {&summon, HACKLIB_DAMAGE_PERIODIC, 9,
                                  2,       HACKLIB_ELEMENT_NONE,    FALSE};
    source.elements = HACKLIB_ELEMENT_FIRE;
    source.elementsKnown = TRUE;
    hackLib_swapDamageSource(&source);
    HackLibCombatEvent miss =
        hackLib_captureCombatEvent(&attacker, &victim, (HackLibDamageResult){0}, FALSE);
    source.abilityRow = 3;
    source.elements = HACKLIB_ELEMENT_POISON;
    assert(miss.source.abilityRow == 9 && miss.source.kind == HACKLIB_DAMAGE_PERIODIC);
    assert(miss.source.elementsKnown && miss.source.elements == HACKLIB_ELEMENT_FIRE);
    assert(hackLib_dispatchCombatEvent(&miss, &handlers, &order));
    assert(kills == 1 && hits == 1 && resolutions == 2);
    assert(hackLib_currentDamageSource() == &source);

    const HackLibCombatHandlers recursive = {NULL, NULL, nested};
    assert(hackLib_dispatchCombatEvent(&miss, &recursive, NULL));
    assert(resolutions == 2 + HACKLIB_MAX_DAMAGE_DEPTH && nesting == 0);
    assert(hackLib_currentDamageSource() == &source);
    miss.source.depth = HACKLIB_MAX_DAMAGE_DEPTH;
    assert(!hackLib_dispatchCombatEvent(&miss, &recursive, NULL));
    assert(!hackLib_dispatchCombatEvent(NULL, &handlers, NULL));
    hackLib_swapDamageSource(NULL);
    // The depth guard must reset after a nested dispatch finishes.
    miss.source.depth = 0;
    assert(hackLib_dispatchCombatEvent(&miss, &handlers, &order));
    assert(hackLib_currentDamageSource() == NULL);
    puts("PASS: combat snapshots, callback ordering, misses, attribution and bounded recursion");
    return 0;
}
