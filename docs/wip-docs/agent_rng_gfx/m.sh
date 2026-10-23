#!/bin/bash
# m.sh Func file...   (run from rotk)
export PATH="~/AppData/Local/rotk-tools/arm-gcc/bin:~/AppData/Local/rotk-tools/armips:$PATH"
cd ~/Desktop/lotr/rotk
f=$1; shift
for c in "$@"; do echo -n "$c: "; python local/wip-docs/agent_rng_gfx/t.py $f $c; done
