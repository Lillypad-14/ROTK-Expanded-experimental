#include "eeprom.h"
#include "variables.h"

/**
 * Select the timer channel (0-3) that drives the timeout IRQ and hand back the handler address.
 *
 * @romaddress 0x0804dce4
 */
u32 eeprom_configureTimeoutTimer(u8 channel, void **callbackOut)
{
    if (channel > 3) {
        return 1;
    }
    g_EepromChannel = channel;
    g_EepromTimerReg = (u16 *)(0x04000100 + g_EepromChannel * 4);
    *callbackOut = eeprom_timeoutIrqHandler;
    return 0;
}
