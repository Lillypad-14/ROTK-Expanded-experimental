# actor_applyRenderOscillation (0x08003650) deferred

Matching needs real Actor members (ROM adds each offset to the actor pointer separately); a pointer-cast overlay does not match.
Offsets: x axis phase u16 0x44, step s16 0x48, amplitude s16 0x4c, adds amplitude * math_cosFp16(phase >> 8) to renderCoords.x.
y axis phase u16 0x46, step s16 0x4a, amplitude s16 0x4e, adds amplitude * math_sinFp16(phase >> 8) to renderCoords.y.
Step is read twice: ldrh for the add, ldrsh for the != 0 test.
Overlaps triggerBoxes[2..3] and field_0x4e. Actor.triggerBoxes is used by hacks/patches/debug_cheats/debug_lab.c (x2) and hacks/patches/ringwraith/ringwraith_kit.c, so renaming the member path touches mod code. Held until the user allows it.
Scratch: local/wip-docs/osc/o1.c (throwaway cast, does not match).
