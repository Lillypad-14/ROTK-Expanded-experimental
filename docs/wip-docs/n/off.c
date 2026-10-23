#include "actor.h"
#include <stddef.h>
char a[offsetof(Actor,parentActor)];char b[offsetof(Actor,tickCallback)];char c[offsetof(Actor,type)];char d[offsetof(Actor,spriteFlags)];char e[offsetof(Actor,flags)];
