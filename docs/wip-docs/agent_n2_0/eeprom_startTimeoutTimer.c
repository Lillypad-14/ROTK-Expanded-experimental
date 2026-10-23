#include "eeprom.h"
#include "gba_io.h"
#include "variables.h"

/**
 * Arm the timeout timer from a config.
 *
 * @romaddress 0x0804dd20
 */
void eeprom_startTimeoutTimer(EepromTimeoutConfig *cfg)
{
    u16 *p = (u16 *)cfg;
    *(u16 *)&g_EepromSavedIME = REG_IME;
    REG_IME = 0;
    g_EepromTimerReg[1] = 0;
    REG_IF = 8 << g_EepromChannel;
    REG_IE |= 8 << g_EepromChannel;
    g_EepromTimeoutFlag = 0;
    g_EepromTimeoutCount = *p++;
    *g_EepromTimerReg = *p++;
    g_EepromTimerReg++;
    *g_EepromTimerReg = *p;
    g_EepromTimerReg--;
    REG_IME = 1;
}
