#include "eeprom.h"
#include "gba_io.h"
#include "variables.h"

/**
 * Stop the EEPROM watchdog and restore the master interrupt enable state.
 *
 * @romaddress 0x0804dda8
 */
void eeprom_stopTimeoutTimer(void)
{
    volatile u16 *timer;

    REG_IME = 0;
    timer = g_EepromTimerReg;
    *timer = 0;
    g_EepromTimerReg = ++timer;
    *timer = 0;
    g_EepromTimerReg = --timer;
    REG_IE &= ~(8u << g_EepromChannel);
    REG_IME = g_EepromSavedIME;
}
