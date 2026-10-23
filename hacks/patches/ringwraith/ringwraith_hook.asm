; Native hero hooks and tables generated from hacks/content/heroes.json.

.thumb

RINGWRAITH_HERO_ID equ 8
RINGWRAITH_BASE_HERO equ 2              ; Aragorn
RINGWRAITH_OBJ_PALETTE equ 6

; Player palette in actor_applyTypeDefaults. In: r5 = heroId. Out: r5 = ObjPalettes index,
; r0 = ObjPalettes. r3 holds the OAM byte, so the trampoline jumps through r0 and returns
; through r1.
.align 2
ringwraith_palette_hook:
    ldr  r0, =HeroPaletteIds
    ldrb r5, [r0, r5]
    ldr  r0, =ObjPalettes
    ldr  r1, =(actor_applyTypeDefaults + 0xbe)|1
    bx   r1
    .pool

; Skill tree title hero name. In: r0 = &PLAYER(active).heroId. Out: r0 = name string.
.align 2
ringwraith_skill_tree_name_hook:
    ldrb r0, [r0]
    bl   ringwraith_getHeroName
    ldr  r1, =(scene_skillTree_enter + 0xf4)|1
    bx   r1
    .pool

; Character stats hero name. In: r0 = &PLAYER(active).heroId. Out: r0 = name string.
.align 2
ringwraith_stats_name_hook:
    ldrb r0, [r0]
    bl   ringwraith_getHeroName
    ldr  r1, =0x08027be4|1
    bx   r1
    .pool

RINGWRAITH_START_ITEMS_CASES equ 0x08026460
RINGWRAITH_BACKPACK_CASES equ 0x0802a578
RINGWRAITH_SPAWN_OFFSETS equ 0x0805858c
RINGWRAITH_CAMERA_HEIGHTS equ 0x080585cc
.include "build/hero_tables.asm"
