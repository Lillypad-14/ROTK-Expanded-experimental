#include "eeprom.h"
#include "gba_io.h"
#include "variables.h"

void eeprom_dmaBits(void *src, void *dst, u16 count);

/**
 * Run a DMA3 transfer of count halfwords with interrupts masked and the EEPROM wait state set.
 *
 * @romaddress 0x0804ddec
 */
void eeprom_dmaBits(void *src, void *dst, u16 count)
{
    u16 ime = REG_IME;
    volatile u16 *cnt;

    REG_IME = 0;
    REG_WAITCNT = (0xf8ff & REG_WAITCNT) | g_EepromDescriptor->waitcntBits;
    REG_DMA3SAD = (u32)src;
    REG_DMA3DAD = (u32)dst;
    cnt = (volatile u16 *)&REG_DMA3CNT_L;
    *(volatile u32 *)cnt = DMA_ENABLE | count;
    if (cnt[1] & 0x8000)
    {
        do
        {
        } while (REG_DMA3CNT_H & 0x8000);
    }
    REG_IME = ime;
}
