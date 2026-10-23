#include "input.h"

/**
 * @romaddress 0x08029db0
 */
void input_init(void)
{
    input_enable();
    input_clear();
}
