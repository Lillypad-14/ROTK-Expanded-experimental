#include "actor.h"
#include <stddef.h>
#define O(f) const int o_##f = offsetof(Actor, f);
O(coords) O(velocity) O(renderCoords) O(oam) O(flags) O(type) O(facing) O(listId) O(triggerBoxCount) O(spriteFlags) O(spriteCellIndex) O(sprite) O(recordGroup) O(as) O(currentAnimationFrame) O(spriteAnimation) O(moveSpeed) O(tileNumber) O(actionState) O(variant) O(unitType) O(currentHp)
