#include "lib/actor_ref.h"
#include "sprite.h"

asm(".section .text\n"
    ".balign 4\n.thumb\n.global modRam_originalAllocMain\n.thumb_func\n"
    "modRam_originalAllocMain:\n"
    "push {r4,r5,lr}\nldr r0, =g_ActorPoolMainFreeList\nldr r5, =g_ActorListMain\nmov r2, #0xa4\n"
    "ldr r3, =actor_allocMain + 8\nbx r3\n.pool\n"
    ".balign 4\n.global modRam_originalAllocOverlay\n.thumb_func\n"
    "modRam_originalAllocOverlay:\n"
    "push {r4,lr}\nldr r0, =g_ActorPoolOverlayFreeList\nldr r4, =g_ActorListOverlay\nmov r2, "
    "#0xa4\n"
    "ldr r3, =actor_allocOverlay + 8\nbx r3\n.pool\n"
    ".balign 4\n.global modRam_originalFreeActor\n.thumb_func\n"
    "modRam_originalFreeActor:\n"
    "push {r4-r7,lr}\nmov r7,r10\nmov r6,r9\nmov r5,r8\n"
    "ldr r3, =sprite_free + 8\nbx r3\n.pool\n");

Actor *modRam_originalAllocMain(void);
Actor *modRam_originalAllocOverlay(void);
s32 modRam_originalFreeActor(Actor *actor);

Actor *modRam_allocMain(void)
{
    Actor *actor = modRam_originalAllocMain();
    hackLib_noteActorAllocated(actor);
    return actor;
}

Actor *modRam_allocOverlay(void)
{
    Actor *actor = modRam_originalAllocOverlay();
    hackLib_noteActorAllocated(actor);
    return actor;
}

s32 modRam_freeActor(Actor *actor)
{
    s32 result = modRam_originalFreeActor(actor);
    if (result == 0)
    {
        hackLib_noteActorRetired(actor);
    }
    return result;
}
