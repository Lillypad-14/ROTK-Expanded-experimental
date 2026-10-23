#include "hdr.h"
void eeprom_startTimeoutTimer(EepromTimeoutConfig *cfg)
{
    u16 *reg;
    g_EepromSavedIME = REG_IME;
    REG_IME = 0;
    reg = g_EepromTimerReg;
    reg[1] = 0;
    REG_IF = 8 << g_EepromChannel;
    REG_IE |= 8 << g_EepromChannel;
    g_EepromTimeoutFlag = 0;
    g_EepromTimeoutCount = cfg->overflowCount;
    reg[0] = cfg->reload;
    g_EepromTimerReg = reg + 1;
    reg[1] = cfg->control;
    g_EepromTimerReg = reg;
    REG_IME = 1;
}
