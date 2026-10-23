#include "eeprom.h"
#include "variables.h"

/**
 * Timer-overflow IRQ handler for the post-write Ready poll; raises the timeout flag when the
 * remaining overflow count reaches zero.
 *
 * @romaddress 0x0804dcbc
 */
void eeprom_timeoutIrqHandler(void)
{
    if (*(volatile u16 *)&g_EepromTimeoutCount != 0) {
        g_EepromTimeoutCount--;
        if (g_EepromTimeoutCount == 0) {
            g_EepromTimeoutFlag = 1;
        }
    }
}
