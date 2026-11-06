#include "lib/summon.h"
#include "patches/ringwraith/ringwraith_kit.h"
#include "patches/skill_expansion/skill_expansion_elite.h"

#if HACK_skill_expansion

/** Thralls fight foes near the wraith and otherwise stay at its side. */
void ringwraith_tickThrall(Actor *thrall)
{
    hackLib_escortGhost(thrall, skillExpansion_isFoe);
    actor_npc_tickPassive(thrall);
}

#endif
