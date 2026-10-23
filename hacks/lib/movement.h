#pragma once

#include "actor.h"

/** Require the actor's entire movement box to be on ordinary, in-bounds floor. */
bool hackLib_canLandAt(const Actor *actor, const Vector2Fp16 *position);

/** Search the requested point and nearby 8-pixel offsets, up to 16 pixels away. */
bool hackLib_findLanding(const Actor *actor, const Vector2Fp16 *preferred, Vector2Fp16 *result);
