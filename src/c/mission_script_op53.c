#include "actor.h"
#include "mission.h"
#include "player.h"
#include "variables.h"

/**
 * Script op 53: show the boss HP bar for a living actor.
 *
 * @romaddress 0x08018908
 */
void mission_script_op53(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);

    if (actor)
    {
        u16 hp = actor->currentHp;

        if (actor->currentHp > 0)
        {
            g_BossHpBarActor = actor;
            g_BossHpBarMaxHp = hp;
            g_PlayerHuds[0].flags.p |= PLAYERHUD_FLAG_BOSS_BAR;
        }
    }
}
