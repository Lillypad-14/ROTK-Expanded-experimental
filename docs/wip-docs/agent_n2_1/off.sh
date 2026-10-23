#!/bin/bash
# usage: off.sh Type field ...
export PATH="~/AppData/Local/rotk-tools/arm-gcc/bin:$PATH"
cd ~/Desktop/lotr/rotk
T=$1; shift
{ echo '#include "actor.h"'; echo '#include "player.h"'; echo '#include <stddef.h>'; for f in "$@"; do echo "const int o_${f//[.\[\]]/_} = offsetof($T, $f);"; done; } > local/wip-docs/agent_n2_1/_off.c
arm-none-eabi-gcc -mthumb -Iinclude -Icarved/include -S -o - local/wip-docs/agent_n2_1/_off.c 2>&1 | grep -E "^o_|\.word|error" | paste -d' ' - - | sed 's/:\s*\.word\s*/=/' | tr '\n' ' '; echo
