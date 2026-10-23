#include "font.h"
#include "scene/shop.h"
#include "text.h"

/**
 * Draw one Yes/No confirm option, highlighted when it is the selected one.
 *
 * @romaddress 0x0801fefc
 */
void scene_shop_drawConfirmOption(u32 optionIndex, u32 selectedIndex)
{
    s32 color = 3;
    s32 charbase;
    s32 y;
    char *str;

    if (optionIndex == selectedIndex)
    {
        color = 0;
    }
    charbase = optionIndex * 10 + 0xbf;
    y = optionIndex * 8 + 0x68;
    str = text_getString(optionIndex + 0x4e);
    font_setStyle(FONT_STYLE_8PX_A, color, 0);
    font_drawString(charbase, 0xb8, y, str, 1);
}
