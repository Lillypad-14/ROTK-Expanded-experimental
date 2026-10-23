#pragma once

#include "actor.h"

enum HackLibDamageKind
{
    HACKLIB_DAMAGE_UNKNOWN = 0,
    HACKLIB_DAMAGE_WEAPON = 1,
    HACKLIB_DAMAGE_PROJECTILE = 2,
    HACKLIB_DAMAGE_ABILITY = 3,
    HACKLIB_DAMAGE_SUMMON = 4,
    HACKLIB_DAMAGE_PERIODIC = 5,
    HACKLIB_DAMAGE_SECONDARY = 6,
};

#define HACKLIB_ABILITY_NONE 0xff
#define HACKLIB_MAX_DAMAGE_DEPTH 8

enum HackLibDamageElement
{
    HACKLIB_ELEMENT_NONE = 0,
    HACKLIB_ELEMENT_FIRE = 1 << 0,
    HACKLIB_ELEMENT_POISON = 1 << 1,
    HACKLIB_ELEMENT_MASK = HACKLIB_ELEMENT_FIRE | HACKLIB_ELEMENT_POISON,
};

typedef struct HackLibDamageSource
{
    Actor *origin;
    u8 kind;
    u8 abilityRow;
    u8 depth;
    // Attack properties, independently of HP removed or status application.
    u8 elements;
    // FALSE distinguishes unspecified damage from explicitly non-elemental damage.
    bool elementsKnown;
} HackLibDamageSource;

/** Synchronous context only. Save the returned pointer and restore it before returning. */
const HackLibDamageSource *hackLib_swapDamageSource(const HackLibDamageSource *source);
const HackLibDamageSource *hackLib_currentDamageSource(void);
