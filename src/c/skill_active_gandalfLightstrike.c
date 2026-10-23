#include "fx.h"
#include "player.h"
#include "sfx.h"
#include "skill.h"
#include "variables.h"

/**
 * Cast Lightstrike.
 *
 * @romaddress 0x08020c44
 */
Actor *skill_active_gandalfLightstrike(Actor *playerActor)
{
    u32 playerIndex = playerActor->as.combat.playerIndex;
    s32 level = PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_GANDALF_LIGHTSTRIKE];
    Actor *fx;

    fx = fx_unknown08044000(playerActor, 0, GandalfLightstrikeFxVariants[level - 1],
                            playerActor->facing, 1, 1);
    fx->type = 0x61;
    sfx_play(0x5a);
    return fx;
}
