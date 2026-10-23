#include "eeprom.h"
#include "match_hacks.h"
#include "gba_io.h"
#include "variables.h"

/**
 * Arm the EEPROM write watchdog and enable its timer interrupt.
 *
 * @romaddress 0x0804dd20
 */
void eeprom_startTimeoutTimer(EepromTimeoutConfig *cfg)
{
    volatile u16 *timer;
    volatile u16 *interrupt;
    MATCH_PIN(u16 *, values, "r0");
    MATCH_PIN(u32, irq, "r7");

    g_EepromSavedIME = REG_IME;
    REG_IME = 0;
    timer = g_EepromTimerReg;
    timer[1] = 0;
    interrupt = &REG_IF;
    irq = INTERRUPT_FLAG_TIMER0 << *(volatile u8 *)&g_EepromChannel;
    *interrupt = irq;
    interrupt--;
    *interrupt |= INTERRUPT_FLAG_TIMER0 << *(volatile u8 *)&g_EepromChannel;
    g_EepromTimeoutFlag = 0;
    values = &cfg->overflowCount;
    g_EepromTimeoutCount = *values++;
    *timer = *values;
    g_EepromTimerReg = timer + 1;
    timer[1] = values[1];
    g_EepromTimerReg = timer;
    REG_IME = 1;
}
