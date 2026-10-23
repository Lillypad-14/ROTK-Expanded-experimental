"""Exercise the patch's actual thrall tick with a captured stock-tick boundary."""

from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
STUBS = r'''
#pragma once
#include "types.h"
#include "vector2.h"
#define ACTOR_FLAG_ANIMATION_END (1u << 22)
#define GHOST_BEHAVIOR_DESPAWNING 1
typedef struct Actor Actor;
typedef struct TestGhostState {
    Actor *target;
    Actor *owner;
    u16 lifetime;
} TestGhostState;
struct Actor {
    Actor *next, *parentActor;
    u8 behaviorState, actionState, field_0x64[1];
    s32 actionStateTimer;
    struct { u32 p; } flags;
    struct { s32 x, y; } coords;
    TestGhostState as;
    u8 type, variant;
    bool foe;
};
#define ACTOR_TYPE_NPC 9
#define NPC_KIND_SIEGE_B 26
static struct { Actor *head; } g_ActorListMain __attribute__((unused));
Actor *findNextEnemy(Actor *owner, Actor *cursor);
bool32 skillExpansion_isFoe(Actor *actor);
void actor_npc_tickPassive(Actor *actor);
void ringwraith_tickThrall(Actor *actor);
'''
HARNESS = r'''
#include "stubs.h"
#include "lib/summon.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Actor owner, thrall, enemies[3];
static Actor *head;
static unsigned ticks;
static u8 passedState;
Actor *findNextEnemy(Actor *actor, Actor *cursor) {
    assert(actor == &owner);
    return cursor ? cursor->next : head;
}
bool32 skillExpansion_isFoe(Actor *actor) { return actor->foe; }
void actor_npc_tickPassive(Actor *actor) {
    assert(actor == &thrall);
    ticks++;
    passedState = actor->actionState;
    if (actor->as.lifetime) actor->as.lifetime--;
    else actor->behaviorState = GHOST_BEHAVIOR_DESPAWNING;
}
static void reset(void) {
    unsigned i;
    memset(&thrall, 0, sizeof(thrall));
    memset(enemies, 0, sizeof(enemies));
    ticks = 0;
    head = &enemies[0];
    for (i = 0; i < 3; i++) {
        enemies[i].foe = TRUE;
        enemies[i].coords.x = (30 - 10 * i) * FP16_16_ONE;
        enemies[i].next = i < 2 ? &enemies[i + 1] : NULL;
    }
    thrall.parentActor = &owner;
    thrall.as.target = &enemies[0];
    thrall.as.lifetime = 600;
    thrall.behaviorState = 32;
    thrall.actionState = 3;
    thrall.actionStateTimer = 7;
}
int main(void) {
    unsigned i;
    reset();
    ringwraith_tickThrall(&thrall);
    assert(passedState == 3 && ticks == 1 && thrall.as.lifetime == 599);
    assert(thrall.actionStateTimer == 7);

    reset();
    thrall.flags.p = ACTOR_FLAG_ANIMATION_END;
    ringwraith_tickThrall(&thrall);
    assert(passedState == 8 && thrall.as.lifetime == 599);
    assert(!(thrall.flags.p & ACTOR_FLAG_ANIMATION_END));
    assert((thrall.field_0x64[0] & 2) && thrall.actionStateTimer == 0);

    reset();
    enemies[0].foe = FALSE;
    thrall.actionState = 0x3d;
    ringwraith_tickThrall(&thrall);
    assert(thrall.as.target == &enemies[2] && passedState == 8);

    reset();
    head = NULL;
    enemies[0].foe = FALSE;
    thrall.flags.p = ACTOR_FLAG_ANIMATION_END;
    ringwraith_tickThrall(&thrall);
    assert(thrall.as.target == NULL && passedState == 2);
    assert(thrall.behaviorState != GHOST_BEHAVIOR_DESPAWNING);
    head = &enemies[1];
    ringwraith_tickThrall(&thrall);
    assert(thrall.as.target == &enemies[2]);

    reset();
    for (i = 0; i < 599; i++) {
        thrall.actionState = 3;
        thrall.flags.p |= ACTOR_FLAG_ANIMATION_END;
        ringwraith_tickThrall(&thrall);
        assert(passedState == 8 && thrall.behaviorState == 32);
    }
    assert(thrall.as.lifetime == 1);
    thrall.actionState = 3;
    thrall.flags.p |= ACTOR_FLAG_ANIMATION_END;
    ringwraith_tickThrall(&thrall);
    assert(passedState == 3 && thrall.as.lifetime == 0);
    ringwraith_tickThrall(&thrall);
    assert(thrall.behaviorState == GHOST_BEHAVIOR_DESPAWNING);

    reset();
    thrall.behaviorState = GHOST_BEHAVIOR_DESPAWNING;
    thrall.actionState = 0x3d;
    ringwraith_tickThrall(&thrall);
    assert(passedState == 0x3d && thrall.behaviorState == GHOST_BEHAVIOR_DESPAWNING);
    reset();
    thrall.type = ACTOR_TYPE_NPC;
    thrall.variant = NPC_KIND_SIEGE_B;
    g_ActorListMain.head = &thrall;
    assert(hackLib_nextOwnedGhost(&owner, NULL) == &thrall);
    assert(hackLib_nextOwnedGhost(&enemies[0], NULL) == NULL);
    thrall.as.lifetime = 1;
    assert(hackLib_nextOwnedGhost(&owner, NULL) == NULL);
    thrall.as.lifetime = 600;
    thrall.behaviorState = GHOST_BEHAVIOR_DESPAWNING;
    assert(hackLib_nextOwnedGhost(&owner, NULL) == NULL);
    puts("PASS: repeat attacks, nearest-target acquisition, idle summons, expiry and consumed summons");
    return 0;
}
'''

compiler = shutil.which("clang") or shutil.which("cc")
assert compiler
with tempfile.TemporaryDirectory(prefix="rotk-thrall-tests-") as directory:
    temporary = Path(directory)
    (temporary / "stubs.h").write_text(STUBS)
    for header in (
        "actor.h", "actor/npc.h", "variables.h",
        "patches/ringwraith/ringwraith_kit.h",
        "patches/skill_expansion/skill_expansion_elite.h",
    ):
        path = temporary / header
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('#include "stubs.h"\n')
    library = temporary / "lib/summon.h"
    library.parent.mkdir(parents=True, exist_ok=True)
    source = (ROOT / "hacks/lib/summon.h").read_text()
    source = source.replace('#define findNextEnemy ((FindNextEnemyFn)(0x080013a8 | 1))', '')
    library.write_text(source)
    harness = temporary / "test.c"
    harness.write_text(HARNESS)
    executable = temporary / "test.exe"
    subprocess.run([
        compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
        f"-I{temporary}", f"-I{ROOT / 'include'}", "-DHACK_skill_expansion=1",
        str(ROOT / "hacks/patches/ringwraith/ringwraith_thralls.c"),
        str(harness), "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)
