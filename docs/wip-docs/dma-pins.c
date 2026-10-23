#include "eeprom.h"
#include "match_hacks.h"
#include "gba_io.h"
#include "variables.h"

/**
 * Transfer EEPROM serial bits with DMA3 while interrupts are disabled.
 *
 * @romaddress 0x0804ddec
 */
void eeprom_dmaBits(const void *source, void *destination, u16 bitCount)
{
    u16 savedIME = REG_IME;
    MATCH_PIN(u16, waitcnt, "r4");
    MATCH_PIN(volatile u16 *, status, "r1");
    volatile u32 *control;

    REG_IME = 0;
    waitcnt = REG_WAITCNT;
    waitcnt &= ~0x0700;
    waitcnt |= g_EepromDescriptor->waitcntBits;
    REG_WAITCNT = waitcnt;
    REG_DMA3SAD = (u32)source;
    REG_DMA3DAD = (u32)destination;
    control = (volatile u32 *)REG_ADDR_DMA3CNT_L;
    *control = bitCount | DMA_ENABLE;
    status = (volatile u16 *)control + 1;
    if (*status & (DMA_ENABLE >> 16))
    {
        MATCH_PIN(volatile u16 *, busy, "r2");
        MATCH_FRESH(busy, (volatile u16 *)REG_ADDR_DMA3CNT_H);
        while (*busy & (DMA_ENABLE >> 16))
        {
        }
    }
    REG_IME = savedIME;
}
