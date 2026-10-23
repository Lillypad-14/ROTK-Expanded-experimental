#include "fast_waitstates.h"

#include "gba_io.h"

/** WAITCNT bits 2-3: WS0 first-access wait (0 = 4, 1 = 3, 2 = 2, 3 = 8 cycles) */
#define WAITCNT_WS0_FIRST_MASK (3 << 2)
#define WAITCNT_WS0_FIRST_2 (2 << 2)
/** WAITCNT bit 4: WS0 second-access wait (0 = 2, 1 = 1 cycle) */
#define WAITCNT_WS0_SECOND_1 (1 << 4)
/** WAITCNT bit 14: game pak prefetch buffer */
#define WAITCNT_PREFETCH (1 << 14)

void fastWaitstates_apply(void)
{
    // Read-modify-write: the SRAM wait (bits 0-1) that mod_save relies on and the WS1/WS2
    // fields stay as the boot code set them.
    REG_WAITCNT = (REG_WAITCNT & ~WAITCNT_WS0_FIRST_MASK) | WAITCNT_WS0_FIRST_2 |
                  WAITCNT_WS0_SECOND_1 | WAITCNT_PREFETCH;
}
