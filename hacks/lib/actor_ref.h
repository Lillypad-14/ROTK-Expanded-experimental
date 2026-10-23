#pragma once

#include "actor.h"

typedef struct HackLibActorRef
{
    Actor *actor;
    u32 generation;
} HackLibActorRef;

void hackLib_noteActorAllocated(Actor *actor);
void hackLib_noteActorRetired(Actor *actor);
HackLibActorRef hackLib_actorRef(Actor *actor);
bool hackLib_isActorRefValid(HackLibActorRef reference);
s32 hackLib_actorCurrentHp(const Actor *actor);
u32 hackLib_actorGeneration(Actor *actor);
