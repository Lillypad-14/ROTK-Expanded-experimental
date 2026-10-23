#define font_setIntTemplateArg hdr_dummy
#include "font.h"
#include "text.h"
#include "variables.h"
#undef font_setIntTemplateArg

/**
 * Format an integer into a template argument slot.
 *
 * @romaddress 0x0801d830
 */
void font_setIntTemplateArg(s32 value, s32 argIndex)
{
    text_formatInt(value, g_FontTemplateArgs[argIndex]);
}
