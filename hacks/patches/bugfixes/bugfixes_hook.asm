; Fixes for bugs in the original game (docs/known-bugs.md). Asm bodies, placed in MEM_HOOK.

; ---- Arrow effects of unique bows (known-bugs #1) ----
; The arrow effect setup (0x08026E88) picks its fallback effect word from the equipped arrows,
; or from the weapon's prefix flags when no arrows are equipped. It has no unique branch, so a
; unique bow's word (the unique type's prefixFlags table, indexed by baseIndex, as
; player_setHandFx reads it for melee) is never used. This replaces that pick:
; the weapon's own word is its prefix's, else its unique's; arrows win only when they bring a
; particle effect of their own, or when the weapon has no word.
; In: r10 = the arrow actor, r6 = the weapon item. Out: r5 = the word, r3 = 0, at 0x08026F0C.
BUGFIXES_ARROW_FX_RESUME equ 0x08026f0c
BUGFIXES_AFFIX_PARTICLE equ 0x38000         ; bits 15..17: the trail particle kind
BUGFIXES_ITEM_TYPE_INFO_SIZE equ 40
BUGFIXES_PREFIX_FLAGS_FIELD equ 0xc
BUGFIXES_ITEM_TYPE_UNIQUE equ 0xe

.align 2
bugfixes_arrowFx_hook:
    mov   r5, #0
    ; the weapon's flags table: ItemTypeInfos[type].prefixFlags
    lsl   r1, r6, #28
    lsr   r1, r1, #28
    mov   r2, #BUGFIXES_ITEM_TYPE_INFO_SIZE
    mul   r2, r1
    ldr   r3, =ItemTypeInfos + BUGFIXES_PREFIX_FLAGS_FIELD
    add   r2, r2, r3
    ldr   r2, [r2]
    cmp   r2, #0
    beq   @@ownDone
    ; its prefix index (bits 16..23), else for a unique its base index (bits 8..15)
    lsl   r0, r6, #8
    lsr   r0, r0, #24
    cmp   r0, #0xff
    bne   @@lookup
    cmp   r1, #BUGFIXES_ITEM_TYPE_UNIQUE
    bne   @@ownDone
    lsl   r0, r6, #16
    lsr   r0, r0, #24
@@lookup:
    lsl   r0, r0, #2
    ldr   r5, [r2, r0]
@@ownDone:
    ; the equipped arrows (actor +0x93, -1 for none)
    mov   r0, r10
    add   r0, #0x93
    mov   r1, #0
    ldsb  r0, [r0, r1]
    cmp   r0, #0
    blt   @@done
    ldr   r2, =ArrowAffixFlags
    lsl   r0, r0, #2
    ldr   r0, [r2, r0]
    cmp   r5, #0
    beq   @@useArrow
    ldr   r1, =BUGFIXES_AFFIX_PARTICLE
    tst   r0, r1
    beq   @@done
@@useArrow:
    mov   r5, r0
@@done:
    mov   r3, #0
    ldr   r0, =BUGFIXES_ARROW_FX_RESUME|1
    bx    r0
    .pool
