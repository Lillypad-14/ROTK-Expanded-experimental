#include "gba_io.h"
#include "save.h"
#include "variables.h"

/**
 * Restore the display status register saved by save_eeprom_readBegin.
 *
 * @romaddress 0x0803cfb4
 */
void save_eeprom_readEnd(void)
{
    REG_DISPSTAT = g_SavedDispstat;
}
