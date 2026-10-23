#include "eeprom.h"
#include "gba_io.h"
#include "variables.h"

/**
 * @romaddress 0x0804ddec
 */
void eeprom_dmaBits(void *src, void *dst, u16 count)
{
    u16 savedIme = REG_IME;

    REG_IME = 0;
    REG_WAITCNT = (REG_WAITCNT & 0xf8ff) | g_EepromDescriptor->waitcntBits;
    REG_DMA3SAD = (u32)src;
    REG_DMA3DAD = (u32)dst;
    *(volatile u32 *)REG_ADDR_DMA3CNT_L = count | 0x80000000;
    if (REG_DMA3CNT_H & 0x8000)
    {
        do
        {
        } while (REG_DMA3CNT_H & 0x8000);
    }
    REG_IME = savedIme;
}
