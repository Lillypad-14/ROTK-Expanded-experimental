#include "lib/summon.h"
#include "patches/ringwraith/ringwraith_kit.h"
#include "patches/skill_expansion/skill_expansion_elite.h"

#if HACK_skill_expansion

/** Recover after attacks and acquire new foes until the thrall's lifetime expires. */
void ringwraith_tickThrall(Actor *thrall)
{
    hackLib_tickPersistentGhost(thrall, skillExpansion_isFoe, findNextEnemy);
}

#endif
