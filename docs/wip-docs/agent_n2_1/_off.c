#include "actor.h"
#include "player.h"
#include <stddef.h>
const int o_coords = offsetof(Actor, coords);
const int o_velocity = offsetof(Actor, velocity);
const int o_renderCoords = offsetof(Actor, renderCoords);
const int o_oam = offsetof(Actor, oam);
const int o_flags = offsetof(Actor, flags);
const int o_type = offsetof(Actor, type);
const int o_facing = offsetof(Actor, facing);
const int o_listId = offsetof(Actor, listId);
const int o_triggerBoxCount = offsetof(Actor, triggerBoxCount);
const int o_spriteFlags = offsetof(Actor, spriteFlags);
const int o_spriteCellIndex = offsetof(Actor, spriteCellIndex);
const int o_sprite = offsetof(Actor, sprite);
const int o_recordGroup = offsetof(Actor, recordGroup);
const int o_as = offsetof(Actor, as);
const int o_currentAnimationFrame = offsetof(Actor, currentAnimationFrame);
const int o_spriteAnimation = offsetof(Actor, spriteAnimation);
const int o_moveSpeed = offsetof(Actor, moveSpeed);
const int o_tileNumber = offsetof(Actor, tileNumber);
const int o_actionState = offsetof(Actor, actionState);
const int o_variant = offsetof(Actor, variant);
const int o_currentHp = offsetof(Actor, currentHp);
