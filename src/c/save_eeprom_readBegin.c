#include "eeprom.h"
#include "gba_io.h"
#include "save.h"
#include "variables.h"

/**
 * Prepare the EEPROM for a read: stash REG_DISPSTAT, acknowledge all interrupts, disable the
 * DISPSTAT IRQs, and arm the timeout timer.
 *
 * @romaddress 0x0803cf78
 */
void save_eeprom_readBegin(void)
{
    g_SavedDispstat = REG_DISPSTAT;
    REG_IF = 0x3fff;
    REG_DISPSTAT = 0;
    eeprom_selectType(4);
    eeprom_configureTimeoutTimer(2, &g_UnknownEepromTimerCallback_030040d8);
}
