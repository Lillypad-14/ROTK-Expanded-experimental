#pragma once

#include "actor.h"

typedef bool32 (*HackLibTargetFilter)(Actor *target, u32 argument);

typedef struct HackLibTargetQuery
{
    Actor *center;
    Actor *exclude;
    HackLibTargetFilter filter;
    u32 argument;
    u16 radius;
    // Zero disables the cone. Otherwise each component must be -1, 0 or 1.
    Vector2Int facing;
    // Cosine of the cone half-angle, scaled by 256. Valid values are 0..256.
    u16 coneCosine;
} HackLibTargetQuery;

Actor *hackLib_nextTarget(const HackLibTargetQuery *query, Actor *cursor);
Actor *hackLib_nearestTarget(const HackLibTargetQuery *query);
bool hackLib_isLiveActor(const Actor *actor);

// World-position queries use the gameplay actor list and exclude removed or dead actors.
Actor *hackLib_nextTargetAt(const Vector2Fp16 *position, Actor *exclude, u16 radius, Actor *cursor);
