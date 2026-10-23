#pragma once

#include "actor.h"

/** Create a stock projectile in an 8-way or 32-way direction. Returns NULL on pool exhaustion. */
Actor *actor_projectile_create(Actor *owner, u8 direction, bool direction32);
