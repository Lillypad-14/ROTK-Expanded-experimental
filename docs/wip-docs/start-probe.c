#include "eeprom.h"
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
    volatile u16 *values;
    s32 irq;

    u32 zero = 0;

    g_EepromSavedIME = REG_IME;
    REG_IME = zero;
    timer = g_EepromTimerReg;
    timer[1] = zero;
    interrupt = &REG_IF;
    irq = INTERRUPT_FLAG_TIMER0 << *(volatile u8 *)&g_EepromChannel;
    *interrupt = irq;
    interrupt--;
    *interrupt |= INTERRUPT_FLAG_TIMER0 << *(volatile u8 *)&g_EepromChannel;
    g_EepromTimeoutFlag = zero;
    values = &cfg->overflowCount;
    g_EepromTimeoutCount = *values++;
    *timer = *values;
    g_EepromTimerReg = timer + 1;
    timer[1] = values[1];
    g_EepromTimerReg = timer;
    REG_IME = 1;
}
