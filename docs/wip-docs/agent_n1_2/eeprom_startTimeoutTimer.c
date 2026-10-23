#define g_EepromSavedIME hdr_dummy
#include "eeprom.h"
#include "gba_io.h"
#include "variables.h"
#undef g_EepromSavedIME
// Needs u16 in variables (data.cfg g_EepromSavedIME u32 -> u16).
extern u16 g_EepromSavedIME;

/**
 * Arm the timeout timer from cfg and clear the timeout flag.
 *
 * @romaddress 0x0804dd20
 */
void eeprom_startTimeoutTimer(EepromTimeoutConfig *cfg)
{
    u16 *p = (u16 *)cfg;
    g_EepromSavedIME = REG_IME;
    REG_IME = 0;
    g_EepromTimerReg[1] = 0;
    REG_IF = 8 << g_EepromChannel;
    REG_IE |= 8 << g_EepromChannel;
    g_EepromTimeoutFlag = 0;
    g_EepromTimeoutCount = *p++;
    *g_EepromTimerReg++ = *p++;
    *g_EepromTimerReg-- = *p;
    REG_IME = 1;
}
