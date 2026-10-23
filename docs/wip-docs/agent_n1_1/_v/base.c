#define eeprom_stopTimeoutTimer hdr_dummy
#include "eeprom.h"
#include "gba_io.h"
#include "variables.h"
#undef eeprom_stopTimeoutTimer

/**
 * Disarm the timeout timer and restore IME and the IE bit.
 *
 * @romaddress 0x0804dda8
 */
void eeprom_stopTimeoutTimer(void)
{
    REG_IME = 0;
    *g_EepromTimerReg++ = 0;
    *g_EepromTimerReg-- = 0;
    REG_IE &= ~(8 << g_EepromChannel);
    REG_IME = *(u16 *)&g_EepromSavedIME;
}
