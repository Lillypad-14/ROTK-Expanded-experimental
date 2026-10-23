#define save_eeprom_readEnd hdr_dummy
#include "gba_io.h"
#include "save.h"
#include "variables.h"
#undef save_eeprom_readEnd

/**
 * Restore the display status register saved by save_eeprom_readBegin.
 *
 * @romaddress 0x0803cfb4
 */
void save_eeprom_readEnd(void)
{
    REG_DISPSTAT = g_SavedDispstat;
}
