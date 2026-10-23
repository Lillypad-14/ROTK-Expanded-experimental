#include "mission.h"
#include "variables.h"
/**
 * Script op 25: set the loot tier.
 *
 * @romaddress 0x08018664
 */
void mission_script_op25(const u8 *command)
{
    g_LootTier = command[4];
}
