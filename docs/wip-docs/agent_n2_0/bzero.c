#define bzero hdr_dummy
#include "libc.h"
#undef bzero

/**
 * Library routine (old_agbcc).
 *
 * @romaddress 0x0804fb10
 */
void bzero(void *dest, size_t n)
{
    char *p = dest;
    size_t i = n;
    while (i--)
    {
        *p++ = 0;
    }
}
