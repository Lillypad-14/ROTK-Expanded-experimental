#include "memory.h"

/**
 * Insert a block after another in the address-ordered block list.
 *
 * @romaddress 0x08032edc
 */
void linkBlock(HeapBlock *prev, HeapBlock *block)
{
    HeapBlock *next;
    block->prev = prev;
    next = prev->next;
    block->next = next;
    prev->next = block;
    if (next != NULL)
    {
        next->prev = block;
    }
}
