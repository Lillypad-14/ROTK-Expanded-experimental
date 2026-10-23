#include "eeprom.h"
#include "gba_io.h"
#include "variables.h"

/**
 * @romaddress 0x0804dda8
 */
void eeprom_stopTimeoutTimer(void)
{
    u8 ch;
    REG_IME = 0;
    *g_EepromTimerReg++ = 0;
    *g_EepromTimerReg-- = 0;
    @@
    REG_IME = *(u16 *)&g_EepromSavedIME;
}
