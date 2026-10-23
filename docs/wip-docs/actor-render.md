# Actor render-follow and oscillation

Promoted actor_updateRenderFollow, 0x08003540, 108 bytes, and actor_applyRenderOscillation, 0x08003650, 112 bytes. Two functions add 220 compiled bytes.

Render-follow copies target renderCoords and OAM bgPriority. SpriteFlags bit2 adds one pixel to render Y, bit3 subtracts one pixel, with bit2 taking precedence. Corrected older comments that incorrectly numbered these bits as3/4. Original and normal C differ only in the second and/cmp destination register; flags pinned to r1 matches, pin removed gives two differences. Byte-width flags and direct memory reload source variants did not remove that difference.

Oscillation fields overlay trigger-box scratch: phaseX/Y at0x44/46, stepX/Y signed halfwords at0x48/4a, amplitudeX/Y signed halfwords at0x4c/4e. The upper phase byte drives cosine for X and sine for Y, multiplied directly by signed amplitude into fixed-point renderCoords. Nonzero step enables each axis independently. Function matches plain C.

ActorTriggerScratch and ActorTriggerScratchView are global parallel typed views; Actor layout and existing field accesses are unchanged. The view starts its scratch window at0x34, encompassing the two preceding bytes, four trigger boxes and the final halfword. Natural union alignment is four bytes; this start avoids inserting padding that a0x36 start caused. A packed union prevented the needed addressing fold. Direct nested component references preserve full actor-relative offsets and avoid caching a separate scratch base across trig calls.

Current inventory:567 C definitions,249 C regions,72,132 compiled bytes,883 mapped symbols. actor-render-check.log passed all451 manifest regions with original SHA1, source/store consistency, BUGFIX compile and297 tests with three default skips. actor-render-ghidra-tests.log passed18 configured tests.

Next actor_tickAllInList evidence: literals at0x0800075c/760 contain code entry addresses0x03002c30 and0x03002d30. These are direct IWRAM entry constants, not pointer-valued RAM globals. actor_checkCollisions at0x0800cd5c is ARM code, but no direct xrefs were found in the current project. Recover the IWRAM copy/relocation setup before assigning names to these entries. Other queue literals match existing named fixed/world/post-commit queues. Current Ghidra RAM at0x03002c30 is unreadable, so do not attempt to disassemble runtime bytes there from this static project.

A raw disassemble_bytes request at0x080035ac used ARM context and returned nonsense; the unimplemented gap before0x08003650 needs Thumb context recovery through Ghidra, not acceptance of that output.
