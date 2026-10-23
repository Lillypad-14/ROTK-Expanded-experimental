#define save_stampLanguage hdr_dummy
#include "save.h"
#include "text.h"
#include "variables.h"
#undef save_stampLanguage

/**
 * Store the current language in the global save header.
 *
 * @romaddress 0x0803cd50
 */
bool save_stampLanguage(void)
{
    g_SaveGlobals.d.language = text_getLanguage();
    return g_SaveGlobals.d.languageSet = 1;
}
