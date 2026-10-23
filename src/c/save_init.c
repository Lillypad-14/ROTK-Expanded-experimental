#include "save.h"
#include "text.h"
#include "variables.h"

/**
 * Boot-time save setup: formats the save header and all four slots when the header is
 * invalid, then applies the saved language and refreshes the slot summaries.
 *
 * @romaddress 0x0803cc30
 */
void save_init(void)
{
    u8 i;

    if (!save_validateHeader())
    {
        save_formatHeader();
        for (i = 0; i < 4; i++)
        {
            save_initEmptySlot(&g_SaveSlots[i]);
            save_writeSlot(&g_SaveSlots[i], i);
        }
    }
    text_setLanguage(g_SaveGlobals.d.language);
    save_refreshSlots();
    g_LastSaveSlot = 0;
}
