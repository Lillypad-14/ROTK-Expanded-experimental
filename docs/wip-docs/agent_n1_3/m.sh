#!/bin/bash
cd ~/Desktop/lotr/rotk
export PATH="~/AppData/Local/rotk-tools/arm-gcc/bin:~/AppData/Local/rotk-tools/armips:$PATH"
uv run rotkit match $1 local/wip-docs/agent_n1_3/$1.c ${2:---full} 2>&1
