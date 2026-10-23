#pragma once

// Fast Waitstates: cartridge ROM (WS0) first-access wait 3 -> 2 cycles. Pure memory timing,
// so game logic is unchanged and only CPU-bound lag frames get rarer.

/** Boot task (runTasks): apply the faster WS0 timing, keeping every other WAITCNT field. */
void fastWaitstates_apply(void);
