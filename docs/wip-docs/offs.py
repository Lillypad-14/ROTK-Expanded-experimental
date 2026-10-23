import sys,subprocess
# compile a tiny probe printing offsetof for fields of Actor
fields=sys.argv[1:]
src='#include "actor.h"\n'+''.join(f'char o_{f}[__builtin_offsetof(Actor,{f})];\n' for f in fields)
open('local/wip-docs/offs_probe.c','w').write(src)
