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
    u8 channel;

    REG_IME = 0;
    timer = g_EepromTimerReg;
    *timer = 0;
    g_EepromTimerReg = ++timer;
    *timer = 0;
    g_EepromTimerReg = --timer;
    channel = g_EepromChannel;
    REG_IE &= ~(INTERRUPT_FLAG_TIMER0 << channel);
    REG_IME = g_EepromSavedIME;
}
